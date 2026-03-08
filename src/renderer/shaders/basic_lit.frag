#version 330 core

in vec2 uv;
in vec2 fragWorldPos;   // World position of this fragment
in float fragDepth;     // Z-depth of this sprite (-1 to 1)

uniform sampler2D texture0;

// Point light constants
#define MAX_LIGHTS 8
uniform int numLights;
uniform vec2 lightPositions[MAX_LIGHTS];
uniform float lightDepths[MAX_LIGHTS];     // 0 to 1 range (0=far, 1=near)
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
    
    // Start with base color
    vec3 finalColor = texColor.rgb;
    
    // Apply point lights
    vec3 lightingColor = vec3(0.0);
    
    // Convert sprite z-depth from [-1, 1] to [0, 1] for comparison
    float spriteDepthNorm = (fragDepth + 1.0) * 0.5;
    
    for (int i = 0; i < numLights; i++) {
        if (i >= MAX_LIGHTS) break;
        
        // Only apply light if it's between camera and this sprite
        // Light must be closer to camera (higher depth value) than sprite
        if (lightDepths[i] < spriteDepthNorm) {
            continue;  // Light is farther away - no effect
        }
        
        // Calculate distance from fragment to light
        vec2 toLight = lightPositions[i] - fragWorldPos;
        float distance = length(toLight);
        
        // Check if fragment is within light radius
        if (distance <= lightRadii[i]) {
            // Calculate falloff
            float falloff = 1.0 - (distance / lightRadii[i]);
            falloff = falloff * falloff;  // Quadratic falloff
            
            // Accumulate colored lighting
            lightingColor += lightColors[i] * lightIntensities[i] * falloff;
        }
    }
    
    // Apply lighting - brighten with colored light
    finalColor = mix(finalColor, finalColor + lightingColor, 0.6);
    finalColor = clamp(finalColor, vec3(0.0), vec3(1.0));
    
    FragColor = vec4(finalColor, texColor.a);
}
