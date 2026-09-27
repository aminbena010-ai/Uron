#version 450

layout(location = 0) in  vec3 fragNormal;
layout(location = 1) in  vec2 fragUV;
layout(location = 2) in  vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(fragNormal);
    vec3 lightDir = normalize(vec3(0.4, -0.8, 0.5));
    float diff = max(dot(n, -lightDir), 0.0);
    vec3 lit = fragColor.rgb * (0.25 + 0.75 * diff);
    outColor = vec4(lit, fragColor.a);
}
