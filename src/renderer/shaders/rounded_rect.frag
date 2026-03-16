#version 330 core

uniform vec4 rectColor;
uniform vec2 rectSize;  // actual size in pixels (width, height)
uniform float radius;   // corner radius in pixels

in vec2 fragLocal;  // [-0.5, 0.5] normalized quad coordinates
out vec4 FragColor;

// Signed distance function for a rounded rectangle.
// p: position relative to rect center
// b: half-extents of the rect (width/2, height/2)
// r: corner radius in the same units as p and b
float roundedBoxSDF(vec2 p, vec2 b, float r) {
    vec2 d = abs(p) - b + vec2(r);
    return length(max(d, 0.0)) - r + min(max(d.x, d.y), 0.0);
}

void main() {
    // Map fragLocal [-0.5, 0.5] into pixel space so radius is in pixels
    vec2 p = fragLocal * rectSize;
    vec2 b = rectSize * 0.5;

    float dist = roundedBoxSDF(p, b, radius);

    // Anti-alias over ~1 pixel at the edge
    float aa = fwidth(dist);
    float alpha = smoothstep(0.0, aa, -dist);

    FragColor = vec4(rectColor.rgb, rectColor.a * alpha);
}
