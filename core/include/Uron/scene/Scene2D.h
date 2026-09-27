#pragma once
#include <Uron/scene/Node2D.h>
#include <Uron/render/Color.h>
#include <memory>
#include <string>

namespace Uron {

class Renderer;

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
    bool m_active = false;
};

}