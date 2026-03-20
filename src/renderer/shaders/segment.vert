#version 330 core

// Raw pre-computed OpenGL positions — no scaling, no uniform transform.
// Used for oriented geometry (cable segments, etc.) where the C++ side
// computes all four corners in OpenGL space directly.
layout(location = 0) in vec2 aPos;

uniform float spriteZ;

void main() {
    gl_Position = vec4(aPos, spriteZ, 1.0);
}
