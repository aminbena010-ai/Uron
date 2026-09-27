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

    // Proyección*visión para drawMesh (identidad por defecto: entonces la
    // transform del drawMesh es el MVP completo). Persiste entre frames.
    virtual void setViewProjection(const Mat4& viewProj) = 0;

    // Dibuja una malla indexada con profundidad. transform = modelo;
    // MVP = viewProjection * transform.
    virtual void drawMesh(const Mesh& mesh, const Mat4& transform) = 0;

    // transform = matriz afín 2D que mapea el quad unitario [0,1]² a
    // coordenadas de pantalla (pixeles, origen arriba-izquierda, Y hacia
    // abajo). El renderer hace la conversión a NDC internamente.
    virtual void drawSprite(const Texture& tex, const Mat4& transform,
                            Shader* shader = nullptr,
                            const Color& tint = Color::White()) = 0;

    virtual void waitIdle() = 0;

    virtual u32 width()  const = 0;
    virtual u32 height() const = 0;
};

}