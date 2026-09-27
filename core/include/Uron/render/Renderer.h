// ============================================================================
//  render/Renderer.h
//  ---------------------------------------------------------------------------
//  QUE ES: Interfaz abstracta del renderizador.
//  CONTIENE: clase Renderer con beginFrame(), clear(), endFrame(),
//            drawMesh(), drawSprite(), setViewport().
//  PARA QUE: Que el motor tenga UNA interfaz de render, y detras pueda
//            haber Vulkan hoy y WebGPU mañana sin que el usuario lo note.
//  QUIEN LO USA: El usuario (a traves de Engine), los plugins.
//  NOTA: Renderer es una INTERFAZ. VulkanRenderer la implementa.
//        El usuario nunca ve VulkanRenderer directamente.
// ============================================================================
#pragma once
#include <Uron/render/Color.h>
#include <Uron/math/Mat4.h>

namespace Uron {

class Mesh;
class Texture;
class Shader;
class Window;

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual bool init(Window* window) = 0;
    virtual void shutdown() = 0;

    virtual void beginFrame() = 0;
    virtual void clear(const Color& c) = 0;
    virtual void endFrame() = 0;

    virtual void setViewport(u32 x, u32 y, u32 w, u32 h) = 0;
    virtual void setClearColor(const Color& c) = 0;

    virtual void drawMesh(const Mesh& mesh, const Mat4& transform) = 0;
    virtual void drawSprite(const Texture& tex, const Mat4& transform) = 0;

    virtual void waitIdle() = 0;

    virtual u32 width()  const = 0;
    virtual u32 height() const = 0;
};

}