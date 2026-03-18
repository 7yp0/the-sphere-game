#version 330 core

in vec2 uv;
in vec3 fragPosWorld;  // Fragment position in OpenGL space (X,Y = screen, Z = depth)

// Textures
uniform sampler2D texture0;     // Diffuse texture
uniform sampler2D normalMap;    // Normal map (optional, flat normal if same as diffuse)
uniform sampler2D depthMap;     // Scene depth map for Z reconstruction

// Scene info
uniform float aspectRatio;      // viewport width / height

// Object info
// objectZ = -999.0 means this is the background (use depth map for Z)
// objectZ = actual value means this is a prop (use this Z, check if light is behind)
uniform float objectZ;

// Self-entity index for shadow exclusion (-1 = no exclusion)
uniform int selfEntityIndex;

// Lighting
#define MAX_LIGHTS 12
uniform int numLights;
uniform vec3 lightPositions[MAX_LIGHTS];   // OpenGL coords (-1 to +1)
uniform vec3 lightColors[MAX_LIGHTS];      // RGB (0-1)
uniform float lightIntensities[MAX_LIGHTS];
uniform float lightRadii[MAX_LIGHTS];

// Shadow casters
#define MAX_SHADOW_CASTERS 8
uniform int numShadowCasters;
uniform vec3 shadowCasterPositions[MAX_SHADOW_CASTERS];   // Center position (OpenGL coords)
uniform vec2 shadowCasterSizes[MAX_SHADOW_CASTERS];       // Width, height (OpenGL units)
uniform float shadowCasterZDepths[MAX_SHADOW_CASTERS];    // Z depth of the caster
uniform vec4 shadowCasterUVRanges[MAX_SHADOW_CASTERS];    // minU, minV, maxU, maxV
uniform float shadowCasterAlphaThresholds[MAX_SHADOW_CASTERS];
uniform float shadowCasterIntensities[MAX_SHADOW_CASTERS];
uniform int shadowCasterEntityIndices[MAX_SHADOW_CASTERS]; // For self-exclusion
uniform sampler2D shadowCasterTex0;
uniform sampler2D shadowCasterTex1;
uniform sampler2D shadowCasterTex2;
uniform sampler2D shadowCasterTex3;
uniform sampler2D shadowCasterTex4;
uniform sampler2D shadowCasterTex5;
uniform sampler2D shadowCasterTex6;
uniform sampler2D shadowCasterTex7;

// Projector lights (window lights with cookie textures)
#define MAX_PROJECTOR_LIGHTS 4
uniform int numProjectorLights;
uniform vec3 projectorPositions[MAX_PROJECTOR_LIGHTS];    // Where the window is
uniform vec3 projectorDirections[MAX_PROJECTOR_LIGHTS];   // Direction light travels
uniform vec3 projectorUps[MAX_PROJECTOR_LIGHTS];          // Up vector for orientation
uniform vec3 projectorColors[MAX_PROJECTOR_LIGHTS];       // Light color
uniform float projectorIntensities[MAX_PROJECTOR_LIGHTS]; // Brightness
uniform float projectorFovs[MAX_PROJECTOR_LIGHTS];        // Field of view (radians)
uniform float projectorAspects[MAX_PROJECTOR_LIGHTS];     // Width/Height ratio
uniform float projectorRanges[MAX_PROJECTOR_LIGHTS];      // Max distance
uniform sampler2D projectorCookie0;
uniform sampler2D projectorCookie1;
uniform sampler2D projectorCookie2;
uniform sampler2D projectorCookie3;

// Ambient light
uniform vec3 ambientColor;  // Default ambient color
uniform float ambientIntensity;  // Default ambient intensity

out vec4 FragColor;

// Sample shadow caster texture by index (GLSL doesn't allow array indexing samplers)
vec4 sampleShadowCasterTexture(int index, vec2 uv) {
    if (index == 0) return texture(shadowCasterTex0, uv);
    if (index == 1) return texture(shadowCasterTex1, uv);
    if (index == 2) return texture(shadowCasterTex2, uv);
    if (index == 3) return texture(shadowCasterTex3, uv);
    if (index == 4) return texture(shadowCasterTex4, uv);
    if (index == 5) return texture(shadowCasterTex5, uv);
    if (index == 6) return texture(shadowCasterTex6, uv);
    if (index == 7) return texture(shadowCasterTex7, uv);
    return vec4(0.0);
}

// Ray-Quad intersection test
// Returns true if ray intersects the axis-aligned quad, with UV coordinates at hit point
// rayOrigin: starting point of ray (fragment position in 3D)
// rayDir: direction towards light (NOT normalized! length = distance to light)
// quadCenter: center of the shadow caster quad (OpenGL coords)
// quadSize: width and height of the quad (NOT half-size)
// quadZ: Z depth of the quad plane
// hitUV: output UV coordinates within the quad (0-1 range)
bool rayQuadIntersect(vec3 rayOrigin, vec3 rayDir, vec3 quadCenter, vec2 quadSize, float quadZ, out vec2 hitUV) {
    // Check if ray is parallel to the quad plane (Z = quadZ)
    if (abs(rayDir.z) < 0.0001) {
        return false;
    }
    
    // Find t where ray intersects the Z=quadZ plane
    // t is parametric: 0 = at fragment, 1 = at light
    float t = (quadZ - rayOrigin.z) / rayDir.z;
    
    // Only count intersections BETWEEN fragment and light (0 < t < 1)
    if (t <= 0.0 || t >= 1.0) {
        return false;
    }
    
    // Calculate intersection point on the plane
    vec3 hitPoint = rayOrigin + rayDir * t;
    
    // Check if hit point is within quad bounds (quadSize is full size, not half)
    vec2 halfSize = quadSize * 0.5;
    vec2 localPos = hitPoint.xy - quadCenter.xy;
    
    if (abs(localPos.x) > halfSize.x || abs(localPos.y) > halfSize.y) {
        return false;
    }
    
    // Convert to UV coordinates (0-1 range)
    // Note: Y is flipped because OpenGL Y+ is up, but texture V=0 is at top
    hitUV.x = (localPos.x + halfSize.x) / quadSize.x;
    hitUV.y = 1.0 - (localPos.y + halfSize.y) / quadSize.y;
    
    return true;
}

// Calculate shadow factor for a fragment
// Returns shadow multiplier (0.0 = full shadow, 1.0 = no shadow)
// fragPos: 3D position of fragment
// lightPos: 3D position of light
// excludeIndex: entity index to exclude (for self-shadowing prevention)
float calculateShadow(vec3 fragPos, vec3 lightPos, int excludeIndex) {
    if (numShadowCasters == 0) {
        return 1.0;  // No shadow casters, fully lit
    }
    
    // Direction from fragment towards light (NOT normalized!)
    // This way t=0 at fragment, t=1 at light
    vec3 rayDir = lightPos - fragPos;
    
    // Test ray against each shadow caster
    for (int i = 0; i < numShadowCasters && i < MAX_SHADOW_CASTERS; i++) {
        // Skip self-shadowing
        if (shadowCasterEntityIndices[i] == excludeIndex && excludeIndex >= 0) {
            continue;
        }
        
        // Shadow caster must be between fragment and light (in Z)
        float casterZ = shadowCasterZDepths[i];
        
        // Test ray-quad intersection
        vec2 hitUV;
        if (rayQuadIntersect(fragPos, rayDir, shadowCasterPositions[i], shadowCasterSizes[i], casterZ, hitUV)) {
            // Map hit UV to texture UV range
            vec4 uvRange = shadowCasterUVRanges[i];
            vec2 texUV = vec2(
                mix(uvRange.x, uvRange.z, hitUV.x),  // minU to maxU
                mix(uvRange.y, uvRange.w, hitUV.y)   // minV to maxV
            );
            
            // Sample shadow caster texture
            vec4 casterSample = sampleShadowCasterTexture(i, texUV);
            
            // If alpha is above threshold, we're in shadow
            if (casterSample.a > shadowCasterAlphaThresholds[i]) {
                return shadowCasterIntensities[i];  // Return shadow intensity (darker = lower value)
            }
        }
    }
    
    return 1.0;  // No shadow hit, fully lit
}

// Convert normal from normal map [0,1] to direction [-1,1]
// Standard normal maps use Z=+1 for "towards camera", but our coordinate system
// uses Z=-1 for "towards camera", so we flip the Z component
vec3 decodeNormal(vec3 normalMapSample) {
    vec3 n = normalMapSample * 2.0 - 1.0;
    n.z = -n.z;  // Flip Z to match our coordinate system
    return normalize(n);
}

// Calculate point light contribution
// isObject: true if this is a prop/object, false if background
// surfaceZ: the Z position of the surface (from objectZ or depth map)
vec3 calculatePointLight(int lightIndex, vec3 fragPos3D, vec3 normal, bool isObject, float surfaceZ) {
    vec3 lightPos = lightPositions[lightIndex];
    vec3 lightColor = lightColors[lightIndex];
    float intensity = lightIntensities[lightIndex];
    float radius = lightRadii[lightIndex];
    
    // For OBJECTS (props): Check if light is BEHIND the object
    // In our coordinate system: Z=-1 is near camera, Z=+1 is far
    // If lightZ > surfaceZ, the light is behind the surface
    if (isObject && lightPos.z > surfaceZ) {
        return vec3(0.0);  // Light is behind object - no illumination
    }
    
    // Vector from fragment to light
    vec3 toLight = lightPos - fragPos3D;
    
    // Direction to light (normalized)
    vec3 lightDir = normalize(toLight);
    
    // Diffuse lighting (N·L)
    float NdotL = dot(normal, lightDir);
    
    // For OBJECTS (props/player): surfaces facing away from light don't get lit
    // This ensures physically correct lighting for 3D objects
    if (isObject && NdotL <= 0.0) {
        return vec3(0.0);  // Surface facing away from light
    }
    
    // For BACKGROUND: use absolute value of NdotL
    // The background is a 2D image - we don't want the normal map to create
    // self-shadowing (sections going completely dark). Cast shadows from props
    // will still work because they go through calculateShadow().
    if (!isObject) {
        NdotL = abs(NdotL);
    }
    
    // Distance calculation - account for aspect ratio on X axis
    // This makes the light falloff circular on screen
    vec3 adjustedToLight = toLight;
    adjustedToLight.x *= aspectRatio;
    float distance = length(adjustedToLight);
    
    // Skip if outside light radius
    if (distance > radius) {
        return vec3(0.0);
    }
    
    // Attenuation: smooth falloff
    float attenuation = 1.0 - smoothstep(0.0, radius, distance);
    attenuation = attenuation * attenuation;  // Quadratic falloff
    
    // Add a small amount of wrap lighting for softer gradients
    NdotL = NdotL * 0.8 + 0.2;
    
    return lightColor * intensity * attenuation * NdotL;
}

// Sample projector cookie texture by index
float sampleProjectorCookie(int index, vec2 uv) {
    if (index == 0) return texture(projectorCookie0, uv).r;
    if (index == 1) return texture(projectorCookie1, uv).r;
    if (index == 2) return texture(projectorCookie2, uv).r;
    if (index == 3) return texture(projectorCookie3, uv).r;
    return 0.0;
}

// Calculate projector light contribution (window light with cookie pattern)
// Uses perspective projection to correctly distort the cookie texture onto surfaces
vec3 calculateProjectorLight(int projIndex, vec3 fragPos3D, vec3 normal, bool isObject, float surfaceZ) {
    vec3 projPos = projectorPositions[projIndex];
    vec3 projDir = projectorDirections[projIndex];
    vec3 projUp = projectorUps[projIndex];
    vec3 projColor = projectorColors[projIndex];
    float intensity = projectorIntensities[projIndex];
    float fov = projectorFovs[projIndex];  // Already in radians
    float aspect = projectorAspects[projIndex];
    float range = projectorRanges[projIndex];
    
    // Vector from projector to fragment
    vec3 toFrag = fragPos3D - projPos;
    
    // Distance along projection axis
    float distAlongAxis = dot(toFrag, projDir);
    
    // Fragment must be in front of the projector (in direction of light)
    if (distAlongAxis <= 0.0) {
        return vec3(0.0);
    }
    
    // Skip if beyond range
    if (distAlongAxis > range) {
        return vec3(0.0);
    }
    
    // Build projector coordinate system
    vec3 projRight = normalize(cross(projDir, projUp));
    vec3 projActualUp = cross(projRight, projDir);
    
    // Project fragment onto the projector plane (perpendicular to projDir)
    // This gives us local U,V coordinates
    float localU = dot(toFrag, projRight);
    float localV = dot(toFrag, projActualUp);
    
    // Calculate the spread at this distance based on FOV
    float halfTanFov = tan(fov * 0.5);
    float spreadU = distAlongAxis * halfTanFov * aspect;
    float spreadV = distAlongAxis * halfTanFov;
    
    // Normalize to UV range [0,1]
    float u = (localU / spreadU) * 0.5 + 0.5;
    float v = (localV / spreadV) * 0.5 + 0.5;
    
    // Check if within cookie bounds
    if (u < 0.0 || u > 1.0 || v < 0.0 || v > 1.0) {
        return vec3(0.0);
    }
    
    // Sample cookie texture (flip V for correct orientation)
    float cookieMask = sampleProjectorCookie(projIndex, vec2(u, 1.0 - v));
    
    // Skip if cookie blocks the light
    if (cookieMask < 0.01) {
        return vec3(0.0);
    }
    
    // Direction from fragment to projector (for lighting calculation)
    vec3 lightDir = normalize(projPos - fragPos3D);
    
    // Diffuse lighting (N·L)
    float NdotL = dot(normal, lightDir);
    
    // For objects: skip if facing away
    if (isObject && NdotL <= 0.0) {
        return vec3(0.0);
    }
    
    // For background: use absolute (no self-shadowing from normals)
    if (!isObject) {
        NdotL = abs(NdotL);
    }
    
    // Distance-based attenuation
    float attenuation = 1.0 - smoothstep(0.0, range, distAlongAxis);
    attenuation = attenuation * attenuation;
    
    // Combine everything
    return projColor * intensity * cookieMask * attenuation * max(NdotL, 0.2);
}

void main() {
    // Sample diffuse texture
    vec4 diffuse = texture(texture0, uv);
    
    // Alpha discard for proper depth testing
    if (diffuse.a < 0.5) {
        discard;
    }
    
    // Sample normal map (or use flat normal if not provided)
    vec3 normalSample = texture(normalMap, uv).rgb;
    vec3 normal;
    
    // If normal map is same as diffuse (no normal map), use flat normal
    // Otherwise decode the normal map
    if (normalSample == diffuse.rgb) {
        // No normal map - use flat normal facing camera
        normal = vec3(0.0, 0.0, -1.0);
    } else {
        normal = decodeNormal(normalSample);
    }
    
    // Determine if this is a background or object render
    // objectZ = -999.0 means background (use depth map)
    // objectZ = actual value means object (use that Z)
    bool isObject = (objectZ > -900.0);
    float surfaceZ;
    vec3 fragPos3D;
    
    if (isObject) {
        // For objects: use the provided objectZ
        surfaceZ = objectZ;
        fragPos3D = vec3(fragPosWorld.xy, surfaceZ);
    } else {
        // For background: reconstruct Z from depth map
        vec2 depthUV = vec2(
            (fragPosWorld.x + 1.0) * 0.5,
            1.0 - (fragPosWorld.y + 1.0) * 0.5  // Flip Y for texture coords
        );
        float depthSample = texture(depthMap, depthUV).r;
        
        // Convert depth map value to Z: white(1.0) = -1.0 (near), black(0.0) = +1.0 (far)
        surfaceZ = 1.0 - depthSample * 2.0;
        fragPos3D = vec3(fragPosWorld.xy, surfaceZ);
    }
    
    // Accumulate light contributions
    vec3 totalLight = ambientColor * ambientIntensity;
    
    for (int i = 0; i < numLights && i < MAX_LIGHTS; i++) {
        vec3 lightContribution = calculatePointLight(i, fragPos3D, normal, isObject, surfaceZ);
        
        // Apply shadow if there's any light contribution
        if (length(lightContribution) > 0.001) {
            float shadowFactor = calculateShadow(fragPos3D, lightPositions[i], selfEntityIndex);
            lightContribution *= shadowFactor;
        }
        
        totalLight += lightContribution;
    }
    
    // Add projector lights (window lights with cookie patterns)
    for (int i = 0; i < numProjectorLights && i < MAX_PROJECTOR_LIGHTS; i++) {
        vec3 projContribution = calculateProjectorLight(i, fragPos3D, normal, isObject, surfaceZ);
        
        // Apply shadow if there's any light contribution
        if (length(projContribution) > 0.001) {
            float shadowFactor = calculateShadow(fragPos3D, projectorPositions[i], selfEntityIndex);
            projContribution *= shadowFactor;
        }
        
        totalLight += projContribution;
    }
    
    // Hue shifting: split light into ambient (shadow base) and dynamic (point/projector lights)
    // → Shadows shift toward blue/violet, lit areas toward yellow/red
    vec3 ambientContrib  = ambientColor * ambientIntensity;
    vec3 dynamicContrib  = max(totalLight - ambientContrib, vec3(0.0));

    const vec3 SHADOW_TINT = vec3(0.82, 0.90, 1.22);  // cool blue/violet in shadows
    const vec3 LIGHT_TINT  = vec3(1.18, 1.05, 0.80);  // warm yellow/red in lit areas

    vec3 tintedLight = ambientContrib * SHADOW_TINT + dynamicContrib * LIGHT_TINT;

    // Apply lighting to diffuse color
    vec3 finalColor = diffuse.rgb * tintedLight;

    // Clamp to prevent over-brightening (or allow HDR with tone mapping later)
    finalColor = min(finalColor, vec3(1.0));
    
    FragColor = vec4(finalColor, diffuse.a);
}
