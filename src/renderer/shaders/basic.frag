#version 330 core

in vec2 uv;

uniform sampler2D texture0;

out vec4 FragColor;

void main() {
    vec4 color = texture(texture0, uv);
    
    // Alpha discard for proper depth testing with transparent pixels
    if (color.a < 0.5) {
        discard;
    }
    
    FragColor = color;
}
