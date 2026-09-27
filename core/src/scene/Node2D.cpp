#include <Uron/scene/Node2D.h>
#include <Uron/scene/Scene2D.h>
#include <Uron/Logger.h>
#include <algorithm>

namespace Uron {

Node2D::Node2D() : m_name("Node") {}
Node2D::Node2D(const std::string& name) : m_name(name) {}

Node2D::~Node2D() {
    // Si este nodo esta siendo destruido, sus hijos no deben tocarlo al
    // deligarse: les anulamos el padre antes de que el vector los destruya.
    for (auto& c : m_children) {
        if (c) c->m_parent = nullptr;
    }
    m_children.clear();

    // Borrado directo de un hijo que sigue en el padre: soltar la
    // propiedad (release) sin re-destruir — ya nos estamos destruyendo.
    if (m_parent) {
        auto& sibs = m_parent->m_children;
        for (auto it = sibs.begin(); it != sibs.end(); ++it) {
            if (it->get() == this) {
                it->release();
                sibs.erase(it);
                break;
            }
        }
        m_parent = nullptr;
    }
}

Node2D* Node2D::addChild(std::unique_ptr<Node2D> child) {
    if (!child) return nullptr;
    if (child.get() == this) {
        URON_ERROR("addChild: un nodo no puede ser hijo de si mismo");
        return nullptr;
    }
    for (Node2D* a = this; a; a = a->m_parent) {
        if (a == child.get()) {
            URON_ERROR("addChild: ciclo detectado, " + child->m_name +
                       " es ancestro de " + m_name);
            return nullptr;
        }
    }
    child->m_parent = this;
    Node2D* raw = child.get();
    m_children.push_back(std::move(child));

    if (m_scene) {
        raw->enterTree(*m_scene);
    }
    return raw;
}

std::unique_ptr<Node2D> Node2D::removeChild(Node2D* child) {
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [child](const std::unique_ptr<Node2D>& c) { return c.get() == child; });

    if (it == m_children.end()) return nullptr;

    std::unique_ptr<Node2D> out = std::move(*it);
    out->m_parent = nullptr;
    m_children.erase(it);

    if (m_scene && out->m_scene) {
        out->exitTree(*m_scene);
    }
    return out;
}

void Node2D::updateTree(Scene2D& scene, float dt) {
    onUpdate(scene, dt);

    for (auto& c : m_children) {
        if (c) c->updateTree(scene, dt);
    }
}

void Node2D::renderTree(Renderer& renderer, const Mat4& parentWorld,
                        const Color& parentTint) {
    if (!m_visible) return;

    m_world     = parentWorld * localTransform();
    m_worldTint = {
        parentTint.r * m_tint.r,
        parentTint.g * m_tint.g,
        parentTint.b * m_tint.b,
        parentTint.a * m_tint.a
    };

    onRender(renderer);

    for (auto& c : m_children) {
        if (c) c->renderTree(renderer, m_world, m_worldTint);
    }
}

Mat4 Node2D::localTransform() const {
    return Mat4::translation({m_position.x, m_position.y, 0.f}) *
           Mat4::rotationZ(m_rotation) *
           Mat4::scale({m_scale.x, m_scale.y, 1.f});
}

void Node2D::enterTree(Scene2D& scene) {
    m_scene = &scene;
    onEnter(scene);
    for (auto& c : m_children) {
        if (c) c->enterTree(scene);
    }
}

void Node2D::exitTree(Scene2D& scene) {
    for (auto& c : m_children) {
        if (c) c->exitTree(scene);
    }
    onExit(scene);
    m_scene = nullptr;
}

}