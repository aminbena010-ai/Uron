// ============================================================================
//  render/Mesh.cpp
//  ---------------------------------------------------------------------------
//  Implementacion de MeshBuilder (BUG-002): triangulo, quad, cubo y circulo
//  listos para Renderer::drawMesh. Windings CCW vistos desde fuera (aunque el
//  pipeline de mallas hoy no hace culling: perspective() invierte Y y el MVP
//  puede ser identidad).
// ============================================================================
#include <Uron/render/Mesh.h>
#include <cmath>

namespace Uron {

static_assert(sizeof(Vertex) == 48, "Vertex = 48 bytes (stride Vulkan)");

namespace {

void setVert(Vertex& v, const Vec3& pos, const Vec3& nrm,
             const Vec2& uv, const Color& col) {
    v.position = pos;
    v.normal   = nrm;
    v.uv       = uv;
    v.color    = col;
}

} // namespace

Mesh MeshBuilder::triangle() {
    Mesh m;
    std::vector<Vertex> v(3);
    const Color w = Color::White();
    setVert(v[0], {0.f,  0.8f, 0.f}, {0.f, 0.f, 1.f}, {0.5f, 0.f}, w);
    setVert(v[1], {-0.8f, -0.6f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 1.f}, w);
    setVert(v[2], {0.8f, -0.6f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f}, w);
    m.setVertices(std::move(v));
    m.setIndices({0, 1, 2});
    return m;
}

Mesh MeshBuilder::quad(f32 w, f32 h) {
    Mesh m;
    f32 x = w * 0.5f;
    f32 y = h * 0.5f;
    const Color white = Color::White();

    std::vector<Vertex> v(4);
    // CCW visto desde +Z (y arriba)
    setVert(v[0], {-x, -y, 0.f}, {0.f, 0.f, 1.f}, {0.f, 1.f}, white);
    setVert(v[1], { x, -y, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f}, white);
    setVert(v[2], { x,  y, 0.f}, {0.f, 0.f, 1.f}, {1.f, 0.f}, white);
    setVert(v[3], {-x,  y, 0.f}, {0.f, 0.f, 1.f}, {0.f, 0.f}, white);
    m.setVertices(std::move(v));
    m.setIndices({0, 1, 2, 0, 2, 3});
    return m;
}

Mesh MeshBuilder::cube(f32 size) {
    Mesh m;
    f32 s = size * 0.5f;
    const Color w = Color::White();

    // 6 caras x 4 vertices; winding CCW visto desde fuera de la cara.
    struct Face { Vec3 n; Vec3 p[4]; };
    const Face faces[6] = {
        {{ 0.f,  0.f,  1.f}, {{-s,-s, s}, { s,-s, s}, { s, s, s}, {-s, s, s}}}, // +Z
        {{ 0.f,  0.f, -1.f}, {{ s,-s,-s}, {-s,-s,-s}, {-s, s,-s}, { s, s,-s}}}, // -Z
        {{ 1.f,  0.f,  0.f}, {{ s,-s, s}, { s,-s,-s}, { s, s,-s}, { s, s, s}}}, // +X
        {{-1.f,  0.f,  0.f}, {{-s,-s,-s}, {-s,-s, s}, {-s, s, s}, {-s, s,-s}}}, // -X
        {{ 0.f,  1.f,  0.f}, {{-s, s, s}, { s, s, s}, { s, s,-s}, {-s, s,-s}}}, // +Y
        {{ 0.f, -1.f,  0.f}, {{-s,-s,-s}, { s,-s,-s}, { s,-s, s}, {-s,-s, s}}}, // -Y
    };

    const Vec2 uvs[4] = {{0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f}};

    std::vector<Vertex> v(24);
    std::vector<u32> idx;
    idx.reserve(36);

    for (int f = 0; f < 6; ++f) {
        for (int i = 0; i < 4; ++i) {
            setVert(v[f * 4 + i], faces[f].p[i], faces[f].n, uvs[i], w);
        }
        u32 base = static_cast<u32>(f * 4);
        idx.push_back(base + 0);
        idx.push_back(base + 1);
        idx.push_back(base + 2);
        idx.push_back(base + 0);
        idx.push_back(base + 2);
        idx.push_back(base + 3);
    }

    m.setVertices(std::move(v));
    m.setIndices(std::move(idx));
    return m;
}

Mesh MeshBuilder::circle(f32 radius, u32 segments) {
    Mesh m;
    if (segments < 3) segments = 3;
    const Color w = Color::White();

    std::vector<Vertex> v(segments + 1);
    setVert(v[0], {0.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, {0.5f, 0.5f}, w);

    const f32 step = 6.28318530718f / static_cast<f32>(segments);
    for (u32 i = 0; i < segments; ++i) {
        f32 a = step * static_cast<f32>(i);
        f32 x = std::cos(a) * radius;
        f32 y = std::sin(a) * radius;
        setVert(v[i + 1], {x, y, 0.f}, {0.f, 0.f, 1.f},
                {0.5f + x / (2.f * radius), 0.5f - y / (2.f * radius)}, w);
    }

    std::vector<u32> idx;
    idx.reserve(segments * 3);
    for (u32 i = 0; i < segments; ++i) {
        idx.push_back(0);
        idx.push_back(1 + i);
        idx.push_back(1 + (i + 1) % segments);
    }

    m.setVertices(std::move(v));
    m.setIndices(std::move(idx));
    return m;
}

}
