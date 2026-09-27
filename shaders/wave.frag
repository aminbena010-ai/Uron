#version 450

layout(location = 0) in  vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform Push {
    vec2 position;
    vec2 size;
    vec2 screenSize;
    float time;
    float amplitude;
    vec3 tint;
} pc;

void main() {
    vec4 texColor = texture(texSampler, fragUV);

    float glow = 0.5 + 0.5 * sin(pc.time * 2.0);

    outColor = vec4(texColor.rgb * pc.tint * (0.8 + 0.4 * glow), texColor.a);
}
