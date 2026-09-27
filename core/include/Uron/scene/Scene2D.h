#pragma once
#include <Uron/scene/Node2D.h>
#include <Uron/render/Color.h>
#include <memory>
#include <string>

namespace Uron {

class Renderer;
class Camera2D;

class Scene2D {
public:
    Scene2D();
    explicit Scene2D(const std::string& name);
    ~Scene2D();

    Scene2D(const Scene2D&)            = delete;
    Scene2D& operator=(const Scene2D&) = delete;

    // ==================== Identidad ====================
    const std::string& name() const { return m_name; }
    void setName(const std::string& n) { m_name = n; }

    // ==================== Raíz ====================
    Node2D* root() const { return m_root.get(); }

    // ==================== Fondo ====================
    const Color& clearColor() const { return m_clearColor; }
    void setClearColor(const Color& c) { m_clearColor = c; }

    // ==================== Cámara ====================
    // Cámara activa (no la posee; debe vivir mientras esté referenciada).
    // Sin cámara, el mundo coincide con la pantalla.
    void setCamera(Camera2D* camera) { m_camera = camera; }
    Camera2D* camera() const { return m_camera; }

    // ==================== Ciclo de vida ====================
    void enter();
    void exit();
    void update(float dt);
    void render(Renderer& renderer);

    bool isActive() const { return m_active; }

private:
    std::string m_name;
    std::unique_ptr<Node2D> m_root;
    Color m_clearColor = Color::Black();
    Camera2D* m_camera = nullptr;
    bool m_active = false;
};

}