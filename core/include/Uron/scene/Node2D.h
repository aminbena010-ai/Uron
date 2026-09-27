#pragma once
#include <Uron/Types.h>
#include <Uron/math/Vec2.h>
#include <Uron/math/Mat4.h>
#include <Uron/render/Color.h>

#include <memory>
#include <string>
#include <vector>

namespace Uron {

class Scene2D;
class Renderer;

class Node2D {
public:
    Node2D();
    explicit Node2D(const std::string& name);
    virtual ~Node2D();

    Node2D(const Node2D&)            = delete;
    Node2D& operator=(const Node2D&) = delete;

    // ==================== Identidad ====================
    const std::string& name() const { return m_name; }
    void setName(const std::string& n) { m_name = n; }

    // ==================== Transform ====================
    Vec2 position() const { return m_position; }
    f32  rotation() const { return m_rotation; }
    Vec2 scale()    const { return m_scale;    }

    void setPosition(const Vec2& p) { m_position = p; }
    void setRotation(f32 r)         { m_rotation = r; }
    void setScale(const Vec2& s)    { m_scale = s;    }

    void translate(const Vec2& d)   { m_position += d; }
    void rotate(f32 r)              { m_rotation += r; }

    // ==================== Jerarquía ====================
    Node2D* parent() const { return m_parent; }

    Node2D* addChild(std::unique_ptr<Node2D> child);
    std::unique_ptr<Node2D> removeChild(Node2D* child);

    const std::vector<std::unique_ptr<Node2D>>& children() const {
        return m_children;
    }

    // ==================== Visibilidad ====================
    bool isVisible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }

    // ==================== Color tint ====================
    const Color& tint() const { return m_tint; }
    void setTint(const Color& c) { m_tint = c; }

    // ==================== Ciclo de vida ====================
    virtual void onEnter(Scene2D& scene)   { (void)scene; }
    virtual void onExit(Scene2D& scene)    { (void)scene; }
    virtual void onUpdate(Scene2D& scene, float dt) { (void)scene; (void)dt; }
    virtual void onRender(Renderer& renderer) { (void)renderer; }

    // ==================== Interno ====================
    void updateTree(Scene2D& scene, float dt);
    void renderTree(Renderer& renderer, const Mat4& parentWorld,
                    const Color& parentTint);
    void enterTree(Scene2D& scene);
    void exitTree(Scene2D& scene);

    // Matriz local (traslación·rotación·escala) y resultado de la última
    // propagación del árbol (válida durante onRender/onUpdate).
    Mat4 localTransform() const;
    const Mat4& worldTransform() const { return m_world; }
    const Color& worldTint() const { return m_worldTint; }

protected:
    std::string m_name;
    Vec2 m_position{0.f, 0.f};
    f32  m_rotation = 0.f;
    Vec2 m_scale{1.f, 1.f};
    Color m_tint = Color::White();
    bool m_visible = true;

    Node2D* m_parent = nullptr;
    std::vector<std::unique_ptr<Node2D>> m_children;

    // Escena a la que pertenece el árbol (nullptr = fuera de escena);
    // la usan addChild/removeChild para disparar enter/exit en vivo.
    Scene2D*   m_scene      = nullptr;
    Mat4       m_world;
    Color      m_worldTint  = Color::White();

    friend class Scene2D;
};

}