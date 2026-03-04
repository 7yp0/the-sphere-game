#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform vec2 spritePos;    // world position (center)
uniform vec2 spriteSize;   // width, height
uniform float spriteZ;     // z-depth for layering

out vec2 uv;

void main() {
    // aPos is in range -0.5 to 0.5
    // Scale by spriteSize and translate to spritePos
    vec2 scaled = aPos * spriteSize;
    vec2 final = scaled + spritePos;
    
    gl_Position = vec4(final, spriteZ, 1.0);
    uv = aTexCoord;
}
