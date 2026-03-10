#version 330 core

// =============================================================================
// 2.5D + 3D HYBRID LIGHTING SHADER WITH SCREEN-SPACE SHADOWS
// =============================================================================
// Based on the hybrid rendering system:
//
// FRAGMENT POSITION:
//   - fragWorldPos.xy = OpenGL screen position (-1 to +1)
//   - Z-depth is sampled from DEPTH MAP at this position
//
// LIGHT POSITION (all in OpenGL space -1 to +1):
//   - X = horizontal (-1=left, +1=right)
//   - Y = vertical (-1=bottom, +1=top) - height above scene
//   - Z = depth (-1=near camera, +1=far/background)
//
// DEPTH MAP ENCODING:
//   - White (1.0) = NEAR camera (Z=-1)
//   - Black (0.0) = FAR from camera (Z=+1)
//
// NORMAL MAP (tangent space, standard convention):
//   - R = X direction (0.5 = neutral, 0=left, 1=right)
//   - G = Y direction (0.5 = neutral, 0=down, 1=up)
//   - B = Z direction (pointing out of surface, typically ~1.0 for flat)
//
// SCREEN-SPACE SHADOW RAYS:
//   - Cast ray from fragment toward light in screen-space (XY)
//   - Sample occluder texture along the ray
//   - Z-check: occluder must be CLOSER TO LIGHT than fragment
//   - Z-threshold prevents self-shadowing (objects on surfaces)
//
// UV COORDINATE SYSTEMS (WICHTIG!):
//   - Depth Map (aus Datei): Y-FLIP → uv.y = (1.0 - openglPos.y) * 0.5
//   - Occluder FBO (runtime): NO FLIP → uv.y = (openglPos.y + 1.0) * 0.5
// =============================================================================

in vec2 uv;
in vec2 fragWorldPos;   // OpenGL coordinates (-1 to +1)
in float fragDepth;     // Sprite's Z-depth for layering

uniform sampler2D texture0;     // Diffuse texture
uniform sampler2D normalMap;    // Normal map (tangent space)
uniform sampler2D depthMap;     // Depth map for Z-depth sampling
uniform float aspectRatio;      // width / height (e.g., 512/360 = 1.42)

// Point light uniforms
#define MAX_LIGHTS 8
uniform int numLights;
uniform vec3 lightPositions[MAX_LIGHTS];   // OpenGL coords (-1 to +1)
uniform vec3 lightColors[MAX_LIGHTS];      // RGB (0 to 1)
uniform float lightIntensities[MAX_LIGHTS];
uniform float lightRadii[MAX_LIGHTS];      // OpenGL units (0 to 4 typical)

out vec4 FragColor;

// -----------------------------------------------------------------------------
// Sample Z-depth from depth map
// Input: OpenGL position (-1 to +1)
// Output: Z-depth (-1 = near, +1 = far)
// -----------------------------------------------------------------------------
float sampleZDepthFromDepthMap(vec2 openglPos) {
    // Convert OpenGL coords to UV coords for texture sampling
    // OpenGL X: -1 (left) to +1 (right)  → UV U: 0 to 1
    // OpenGL Y: -1 (bottom) to +1 (top)  → UV V: 1 to 0 (Y is flipped!)
    vec2 uv_coord;
    uv_coord.x = (openglPos.x + 1.0) * 0.5;
    uv_coord.y = (1.0 - openglPos.y) * 0.5;  // Flip Y: top of screen = V=0
    uv_coord = clamp(uv_coord, 0.0, 1.0);
    
    // Sample depth (grayscale)
    float depthValue = texture(depthMap, uv_coord).r;
    
    // Convert to Z-depth:
    // White (1.0) = near camera = Z=-1
    // Black (0.0) = far from camera = Z=+1
    float z = 1.0 - (depthValue * 2.0);
    
    return z;
}

// -----------------------------------------------------------------------------
// Calculate lighting for a single point light
// -----------------------------------------------------------------------------
vec3 calculatePointLight(int lightIndex, vec3 fragPos3D, vec3 normal) {
    vec3 lightPos = lightPositions[lightIndex];
    vec3 lightColor = lightColors[lightIndex];
    float intensity = lightIntensities[lightIndex];
    float radius = lightRadii[lightIndex];
    
    // Vector from fragment to light (in OpenGL 3D space)
    vec3 toLight = lightPos - fragPos3D;
    
    // Correct for aspect ratio: scale X so distance is calculated in "screen space"
    // This makes the light appear circular instead of elliptical
    vec3 toLightCorrected = vec3(toLight.x * aspectRatio, toLight.y, toLight.z);
    float distance = length(toLightCorrected);
    
    // Early exit if outside light radius
    if (distance >= radius) {
        return vec3(0.0);
    }
    
    // Normalize the ORIGINAL direction (not aspect-corrected) for lighting calculation
    vec3 lightDir = normalize(toLight);
    
    // Attenuation: smooth falloff based on corrected distance
    float normalizedDist = distance / radius;
    float attenuation = 1.0 - normalizedDist;
    attenuation = attenuation * attenuation;  // Quadratic falloff
    
    // Diffuse lighting: how much the surface faces the light
    float NdotL = dot(normal, lightDir);
    float diffuse = max(0.0, NdotL);
    
    // Final light contribution
    return lightColor * intensity * attenuation * diffuse;
}

void main() {
    // Sample diffuse texture
    vec4 texColor = texture(texture0, uv);
    
    // Alpha test for transparency
    if (texColor.a < 0.5) {
        discard;
    }
    
    // -------------------------------------------------------------------------
    // Sample and decode normal map
    // -------------------------------------------------------------------------
    // Standard tangent-space normal map:
    //   RGB (0.5, 0.5, 1.0) = flat surface pointing toward camera
    //   R: X-axis (left/right tilt)
    //   G: Y-axis (up/down tilt)  
    //   B: Z-axis (toward camera)
    vec3 normalSample = texture(normalMap, uv).rgb;
    
    // Convert from [0,1] to [-1,+1] range
    vec3 normal;
    normal.x = normalSample.r * 2.0 - 1.0;  // X: left(-1) to right(+1)
    normal.y = normalSample.g * 2.0 - 1.0;  // Y: down(-1) to up(+1)
    normal.z = normalSample.b * 2.0 - 1.0;  // Z: into screen(-1) to toward camera(+1)
    normal = normalize(normal);
    
    // -------------------------------------------------------------------------
    // Calculate fragment's 3D position
    // -------------------------------------------------------------------------
    // X,Y from vertex shader (OpenGL screen coords)
    // Z from depth map sampling
    float fragZ = sampleZDepthFromDepthMap(fragWorldPos);
    vec3 fragPos3D = vec3(fragWorldPos.x, fragWorldPos.y, fragZ);
    
    // -------------------------------------------------------------------------
    // Accumulate lighting from all point lights
    // -------------------------------------------------------------------------
    vec3 totalLight = vec3(0.0);
    
    for (int i = 0; i < numLights && i < MAX_LIGHTS; i++) {
        totalLight += calculatePointLight(i, fragPos3D, normal);
    }
    
    // -------------------------------------------------------------------------
    // Final color: ambient + dynamic lighting
    // -------------------------------------------------------------------------
    vec3 ambientLight = vec3(0.3);  // Base ambient (slightly dark)
    vec3 finalColor = texColor.rgb * (ambientLight + totalLight);
    
    // Clamp to valid range
    finalColor = clamp(finalColor, 0.0, 1.0);
    
    FragColor = vec4(finalColor, texColor.a);
}
