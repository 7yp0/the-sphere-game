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

// Lighting
#define MAX_LIGHTS 8
uniform int numLights;
uniform vec3 lightPositions[MAX_LIGHTS];   // OpenGL coords (-1 to +1)
uniform vec3 lightColors[MAX_LIGHTS];      // RGB (0-1)
uniform float lightIntensities[MAX_LIGHTS];
uniform float lightRadii[MAX_LIGHTS];

// Ambient light
uniform vec3 ambientColor;  // Default ambient color
uniform float ambientIntensity;  // Default ambient intensity

out vec4 FragColor;

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
    
    // For background: allow lighting from any direction (no backface culling)
    // For objects: only light if facing the light
    if (isObject && NdotL <= 0.0) {
        return vec3(0.0);  // Surface facing away from light
    }
    
    // For background, we want the absolute value for N·L since the normal
    // might point "into" the scene but we still want lighting
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
        totalLight += calculatePointLight(i, fragPos3D, normal, isObject, surfaceZ);
    }
    
    // Apply lighting to diffuse color
    vec3 finalColor = diffuse.rgb * totalLight;
    
    // Clamp to prevent over-brightening (or allow HDR with tone mapping later)
    finalColor = min(finalColor, vec3(1.0));
    
    FragColor = vec4(finalColor, diffuse.a);
}
