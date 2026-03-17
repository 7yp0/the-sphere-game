#version 330 core

in vec2 uv;
uniform sampler2D texture0;
uniform vec4 tintColor;
out vec4 FragColor;

void main() {
    vec4 color = texture(texture0, uv);
    // Multiply color by tint
    FragColor = vec4(color.rgb * tintColor.rgb, color.a * tintColor.a);
    // Discard fully transparent pixels to avoid depth buffer pollution
    if (FragColor.a < 0.01) discard;
}
