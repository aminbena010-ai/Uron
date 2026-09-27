#version 450

layout(location = 0) in  vec2 fragUV;
layout(location = 1) in  vec4 fragSpriteTint;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform Push {
    vec4 row0;
    vec4 row1;
    vec4 spriteTint;
    float time;       // offset 48
    float amplitude;  // offset 52
    vec4 tint;        // offset 64
} pc;

void main() {
    vec4 texColor = texture(texSampler, fragUV);

    float glow = 0.5 + 0.5 * sin(pc.time * 2.0);

    outColor = texColor * fragSpriteTint *
               vec4(pc.tint.rgb * (0.8 + 0.4 * glow), pc.tint.a);
}
