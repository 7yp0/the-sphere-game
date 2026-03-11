#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform vec2 spritePos;      // OpenGL position (center)
uniform vec2 spriteSize;     // OpenGL size (width, height)
uniform float spriteZ;       // Z-depth for layering
uniform vec4 texCoordRange;  // min_u, min_v, max_u, max_v

out vec2 uv;
out vec3 fragPosWorld;       // Fragment position in OpenGL space

void main() {
    // aPos is in range -0.5 to 0.5
    // Scale by spriteSize and translate to spritePos
    vec2 scaled = aPos * spriteSize;
    vec2 final = scaled + spritePos;
    
    gl_Position = vec4(final, spriteZ, 1.0);
    
    // Pass world position for lighting calculations
    fragPosWorld = vec3(final, spriteZ);
    
    // Map quad UV (0-1) to texture coordinate range
    uv = mix(texCoordRange.xy, texCoordRange.zw, aTexCoord);
}
