#version 330 core

layout(location = 0) in vec2 aPos;

uniform vec2 spritePos;
uniform vec2 spriteSize;
uniform float spriteZ;

out vec2 fragLocal;

void main() {
    // aPos: [-0.5,0.5] quad (siehe VAO Setup)
    fragLocal = aPos;
    vec2 scaled = aPos * spriteSize; // scale from [-0.5,0.5] to [-size/2, size/2]
    vec2 final = scaled + spritePos;
    gl_Position = vec4(final, spriteZ, 1.0);
}
