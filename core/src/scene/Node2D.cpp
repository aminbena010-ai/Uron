#include <Uron/scene/Node2D.h>
#include <Uron/scene/Scene2D.h>
#include <Uron/Logger.h>
#include <algorithm>

namespace Uron {

Node2D::Node2D() : m_name("Node") {}
Node2D::Node2D(const std::string& name) : m_name(name) {}

Node2D::~Node2D() = default;

Node2D* Node2D::addChild(std::unique_ptr<Node2D> child) {
    if (!child) return nullptr;
    child->m_parent = this;
    Node2D* raw = child.get();
    m_children.push_back(std::move(child));
    return raw;
}

std::unique_ptr<Node2D> Node2D::removeChild(Node2D* child) {
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [child](const std::unique_ptr<Node2D>& c) { return c.get() == child; });

    if (it == m_children.end()) return nullptr;

    std::unique_ptr<Node2D> out = std::move(*it);
    out->m_parent = nullptr;
    m_children.erase(it);
    return out;
}

void Node2D::updateTree(Scene2D& scene, float dt) {
    if (!m_visible) return;

    onUpdate(scene, dt);

    for (auto& c : m_children) {
        if (c) c->updateTree(scene, dt);
    }
}

void Node2D::renderTree(Renderer& renderer) {
    if (!m_visible) return;

    onRender(renderer);

    for (auto& c : m_children) {
        if (c) c->renderTree(renderer);
    }
}

void Node2D::enterTree(Scene2D& scene) {
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
}

}