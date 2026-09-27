#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 vUV;

layout(push_constant) uniform Push {
    vec4 row0;        // a b c d   (afín 2D)
    vec4 row1;        // e f screenW screenH
    vec4 spriteTint;
    float time;       // offset 48 (uniform del ejemplo)
} pc;

void main() {
    // Rotación 2D alrededor del centro del quad (0.5, 0.5)
    vec2 c = inPosition - 0.5;
    float s = sin(pc.time);
    float co = cos(pc.time);
    vec2 p = vec2(c.x * co - c.y * s, c.x * s + c.y * co) + 0.5;

    // Quad unitario -> pixeles -> NDC (mismo contrato que sprite/wave)
    vec2 pix = vec2(dot(pc.row0.xz, p) + pc.row1.x,
                    dot(pc.row0.yw, p) + pc.row1.y);
    vec2 ndc = pix / pc.row1.zw * 2.0 - 1.0;

    gl_Position = vec4(ndc, 0.0, 1.0);
    vUV = inUV;
}
