// ============================================================================
//  render/Mesh.h
//  ---------------------------------------------------------------------------
//  QUE ES: Malla geometrica (vertices + indices).
//  CONTIENE: struct Vertex, struct Mesh, y MeshBuilder para crear mallas
//            (triangulo, quad, cubo, circulo, etc.).
//  PARA QUE: Que el usuario dibuje formas sin tocar buffers de Vulkan.
//  QUIEN LO USA: El usuario (dibujar cosas), el motor (formas basicas).
//  EJEMPLO:
//     Mesh tri = MeshBuilder::triangle();
//     Mesh quad = MeshBuilder::quad(1.f, 1.f);
//     Mesh cube = MeshBuilder::cube(1.f);
// ============================================================================
#pragma once
#include <Uron/Types.h>
#include <Uron/math/Vec2.h>
#include <Uron/math/Vec3.h>
#include <Uron/render/Color.h>
#include <vector>

namespace Uron {

struct Vertex {
    Vec3  position;
    Vec3  normal;
    Vec2  uv;
    Color color;
};

class Mesh {
public:
    Mesh() = default;

    bool isValid() const { return !m_vertices.empty(); }
    const std::vector<Vertex>& vertices() const { return m_vertices; }
    const std::vector<u32>&    indices()  const { return m_indices;  }
    u32 vertexCount() const { return static_cast<u32>(m_vertices.size()); }
    u32 indexCount()  const { return static_cast<u32>(m_indices.size());  }

    void setVertices(std::vector<Vertex> v) { m_vertices = std::move(v); }
    void setIndices(std::vector<u32> i)     { m_indices  = std::move(i); }

    void clear() { m_vertices.clear(); m_indices.clear(); }

private:
    std::vector<Vertex> m_vertices;
    std::vector<u32>    m_indices;

    friend class VulkanRenderer;
};

class MeshBuilder {
public:
    static Mesh triangle();
    static Mesh quad(f32 w = 1.f, f32 h = 1.f);
    static Mesh cube(f32 size = 1.f);
    static Mesh circle(f32 radius = 1.f, u32 segments = 32);
};

}