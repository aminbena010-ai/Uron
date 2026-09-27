#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

void main() {
    // Borde brillante según la UV (gira con el quad)
    float edge = smoothstep(0.0, 0.05, vUV.x) *
                 smoothstep(0.0, 0.05, vUV.y) *
                 smoothstep(0.0, 0.05, 1.0 - vUV.x) *
                 smoothstep(0.0, 0.05, 1.0 - vUV.y);

    vec3 color = mix(vec3(0.1, 0.1, 0.15),
                     vec3(0.2, 0.6, 1.0),
                     edge);

    outColor = vec4(color, 1.0);
}
