#version 330 core

in vec2 uv;

uniform sampler2D texture0;

out vec4 FragColor;

void main() {
    vec4 color = texture(texture0, uv);
    
    // Discard fully transparent pixels to avoid depth buffer pollution
    if (color.a < 0.01) {
        discard;
    }
    
    FragColor = color;
}
