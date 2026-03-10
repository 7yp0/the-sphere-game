#version 410 core

// Simple upscale shader - samples FBO texture with nearest neighbor filtering
// GL_NEAREST filtering is set on the texture, shader just samples and outputs

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;

void main()
{
    FragColor = texture(screenTexture, TexCoord);
}
