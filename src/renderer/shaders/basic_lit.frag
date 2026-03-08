#version 330 core

in vec2 uv;
in vec2 fragWorldPos;   // World position of this fragment
in float fragDepth;     // Z-depth of this sprite (-1 to 1)

uniform sampler2D texture0;
uniform sampler2D normalMap;  // Normal map texture (unit 1)

// Point light constants
#define MAX_LIGHTS 8
uniform int numLights;
uniform vec3 lightPositions[MAX_LIGHTS];   // Full 3D positions
uniform vec3 lightColors[MAX_LIGHTS];
uniform float lightIntensities[MAX_LIGHTS];
uniform float lightRadii[MAX_LIGHTS];

out vec4 FragColor;

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
    
    // Start with base color
    vec3 finalColor = texColor.rgb;
    
    // Apply point lights
    vec3 lightingColor = vec3(0.0);
    
    for (int i = 0; i < numLights; i++) {
        if (i >= MAX_LIGHTS) break;
        
        // Calculate full 3D direction from fragment to light
        // Fragment is at (fragWorldPos.xy, fragDepth in z)
        vec3 fragmentPos = vec3(fragWorldPos, fragDepth);
        vec3 toLight = lightPositions[i] - fragmentPos;
        float distance = length(toLight);
        vec3 lightDir = normalize(toLight);  // Full 3D normalized direction
        
        // Check if fragment is within light radius
        if (distance <= lightRadii[i]) {
            // Calculate falloff
            float falloff = 1.0 - (distance / lightRadii[i]);
            falloff = falloff * falloff;  // Quadratic falloff
            
            // Use true 3D normal dot product with 3D light direction for proper Lambert lighting
            float normalInfluence = dot(normal, lightDir);
            normalInfluence = max(normalInfluence, 0.0);  // Only consider front-facing surfaces
            
            // Accumulate colored lighting with proper diffuse (Lambert) model
            lightingColor += lightColors[i] * lightIntensities[i] * falloff * normalInfluence;
        }
    }
    
    // Apply lighting - brighten with colored light
    finalColor = mix(finalColor, finalColor + lightingColor, 0.6);
    finalColor = clamp(finalColor, vec3(0.0), vec3(1.0));
    
    FragColor = vec4(finalColor, texColor.a);
}
