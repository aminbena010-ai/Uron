#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragUV;

layout(push_constant) uniform Push {
    vec2 position;
    vec2 size;
    vec2 screenSize;
    float time;
    float amplitude;
    vec3 tint;
} pc;

void main() {
    vec2 worldPos = pc.position + inPosition * pc.size;

    float wave = sin(worldPos.x * 0.05 + pc.time * 3.0) * pc.amplitude;
    worldPos.y += wave;

    vec2 ndc = worldPos / pc.screenSize;
    ndc = ndc * 2.0 - 1.0;
    ndc.y = -ndc.y;

    gl_Position = vec4(ndc, 0.0, 1.0);
    fragUV = inUV;
}
