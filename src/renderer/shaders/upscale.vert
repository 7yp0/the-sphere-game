#version 410 core

// Fullscreen quad for upscaling FBO to viewport
// No uniforms needed - covers entire screen

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    // Map quad vertices to fullscreen (-1 to +1)
    gl_Position = vec4(aPos * 2.0, 0.0, 1.0);
    // Flip Y coordinate to correct FBO orientation
    TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
}
