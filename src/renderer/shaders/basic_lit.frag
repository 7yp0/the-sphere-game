#version 330 core

in vec2 uv;
in vec2 fragWorldPos;   // World position of this fragment
in float fragDepth;     // Z-depth of this sprite (-1 to 1)

uniform sampler2D texture0;
uniform sampler2D normalMap;  // Normal map texture (unit 1)
uniform sampler2D heightMap;  // Height map texture (unit 2) - optional
uniform vec2 sceneSize;  // Scene dimensions for height map sampling

// Point light constants
#define MAX_LIGHTS 8
uniform int numLights;
uniform vec3 lightPositions[MAX_LIGHTS];   // Full 3D positions
uniform vec3 lightColors[MAX_LIGHTS];
uniform float lightIntensities[MAX_LIGHTS];
uniform float lightRadii[MAX_LIGHTS];

out vec4 FragColor;

// Calculate Z-depth from height map (same logic as CPU)
float getDepthFromHeightMap(vec2 worldPos) {
    // Normalize world position to texture coordinates [0, 1]
    vec2 texCoord = worldPos / sceneSize;
    texCoord = clamp(texCoord, 0.0, 1.0);
    
    // Sample height value (grayscale - R channel)
    float heightValue = texture(heightMap, texCoord).r;
    
    // Map to Z range: white (1.0) = near (-0.8), black (0.0) = far (0.8)
    // Formula: z = 0.8 - (heightValue * 1.6)
    float z = 0.8 - (heightValue * 1.6);
    
    return z;
}

void main() {
    vec4 texColor = texture(texture0, uv);
    
    // Alpha discard for proper depth testing with transparent pixels
    if (texColor.a < 0.5) {
        discard;
    }
    
    // Sample normal from normal map
    // Normal map format: RGB is (X, Y, Z) in [0, 1] range
    // Convert from [0, 1] to [-1, 1] range for true 3D normal
    vec3 normal = texture(normalMap, uv).rgb;
    normal = normalize(normal * 2.0 - 1.0);  // True 3D normal
    
    // Calculate fragment Z-depth: sample from height map
    float fragmentZ = getDepthFromHeightMap(fragWorldPos);
    
    // DEBUG: Visualize height map Z values (comment out for normal rendering)
    // return vec4(vec3(fragmentZ * 0.5 + 0.5), texColor.a);  // Visualize Z as grayscale
    
    // Start with base color
    vec3 finalColor = texColor.rgb;
    
    // Apply point lights
    vec3 lightingColor = vec3(0.0);
    
    for (int i = 0; i < numLights; i++) {
        if (i >= MAX_LIGHTS) break;
        
        // Get light position
        vec3 lightPos = lightPositions[i];
        
        // Calculate full 3D direction from fragment to light
        // Fragment is at (fragWorldPos.xy, fragmentZ)
        vec3 fragmentPos = vec3(fragWorldPos, fragmentZ);
        vec3 toLight = lightPos - fragmentPos;
        float distance = length(toLight);
        
        // Calculate attenuation based on distance and light radius
        float attenuation = max(0.0, 1.0 - (distance / lightRadii[i]));
        
        // Simple directional lighting: closer = brighter
        lightingColor += lightColors[i] * lightIntensities[i] * attenuation;
    }
    
    // Apply lighting
    finalColor = mix(finalColor, finalColor + lightingColor, 0.6);
    finalColor = clamp(finalColor, vec3(0.0), vec3(1.0));
    
    FragColor = vec4(finalColor, texColor.a);
}
