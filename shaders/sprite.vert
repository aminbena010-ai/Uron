#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragTint;

layout(push_constant) uniform Push {
    vec4 row0;   // a b c d   (afín 2D)
    vec4 row1;   // e f screenW screenH
    vec4 tint;
} pc;

void main() {
    // pixeles, origen arriba-izquierda, Y hacia abajo
    vec2 pix = vec2(dot(pc.row0.xz, inPosition) + pc.row1.x,
                    dot(pc.row0.yw, inPosition) + pc.row1.y);

    vec2 ndc = pix / pc.row1.zw * 2.0 - 1.0;

    gl_Position = vec4(ndc, 0.0, 1.0);
    fragUV = inUV;
    fragTint = pc.tint;
}
