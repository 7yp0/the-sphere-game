#version 330 core

layout(location = 0) in vec2 aPos;

uniform vec2 spritePos;
uniform vec2 spriteSize;
uniform float spriteZ;

void main() {
    vec2 scaled = aPos * spriteSize;
    vec2 final = scaled + spritePos;
    gl_Position = vec4(final, spriteZ, 1.0);
}
