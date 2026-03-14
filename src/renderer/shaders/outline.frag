#version 330 core

in vec2 uv;

uniform sampler2D texture0;
uniform vec2 texelSize;      // 1.0 / texture_size (for neighbor sampling)
uniform vec4 outlineColor;   // Outline color (RGBA)

out vec4 FragColor;

void main() {
    vec4 color = texture(texture0, uv);
    
    // If this pixel is opaque, draw it normally
    if (color.a >= 0.5) {
        FragColor = color;
        return;
    }
    
    // This pixel is transparent - check if any neighbor is opaque
    // Sample 4 direct neighbors (up, down, left, right)
    float neighborAlpha = 0.0;
    neighborAlpha = max(neighborAlpha, texture(texture0, uv + vec2(texelSize.x, 0.0)).a);
    neighborAlpha = max(neighborAlpha, texture(texture0, uv + vec2(-texelSize.x, 0.0)).a);
    neighborAlpha = max(neighborAlpha, texture(texture0, uv + vec2(0.0, texelSize.y)).a);
    neighborAlpha = max(neighborAlpha, texture(texture0, uv + vec2(0.0, -texelSize.y)).a);
    
    // Optional: Also check diagonals for thicker outline
    neighborAlpha = max(neighborAlpha, texture(texture0, uv + vec2(texelSize.x, texelSize.y)).a);
    neighborAlpha = max(neighborAlpha, texture(texture0, uv + vec2(-texelSize.x, texelSize.y)).a);
    neighborAlpha = max(neighborAlpha, texture(texture0, uv + vec2(texelSize.x, -texelSize.y)).a);
    neighborAlpha = max(neighborAlpha, texture(texture0, uv + vec2(-texelSize.x, -texelSize.y)).a);
    
    // If a neighbor is opaque, draw outline
    if (neighborAlpha >= 0.5) {
        FragColor = outlineColor;
    } else {
        discard;  // Transparent and no opaque neighbors
    }
}
