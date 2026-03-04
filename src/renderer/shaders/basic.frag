#version 330 core

in vec2 uv;

uniform sampler2D texture0;

out vec4 FragColor;

void main() {
    FragColor = texture(texture0, uv);
}
