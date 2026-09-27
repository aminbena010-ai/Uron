#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragSpriteTint;

layout(push_constant) uniform Push {
    vec4 row0;        // a b c d   (afín 2D)
    vec4 row1;        // e f screenW screenH
    vec4 spriteTint;
    float time;       // offset 48
    float amplitude;  // offset 52
    vec4 tint;        // offset 64 (uniform del ejemplo)
} pc;

void main() {
    vec2 pix = vec2(dot(pc.row0.xz, inPosition) + pc.row1.x,
                    dot(pc.row0.yw, inPosition) + pc.row1.y);

    float wave = sin(pix.x * 0.05 + pc.time * 3.0) * pc.amplitude;
    pix.y += wave;

    vec2 ndc = pix / pc.row1.zw * 2.0 - 1.0;

    gl_Position = vec4(ndc, 0.0, 1.0);
    fragUV = inUV;
    fragSpriteTint = pc.spriteTint;
}
