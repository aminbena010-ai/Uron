#include <Uron/scene/Scene2D.h>
#include <Uron/scene/Camera2D.h>
#include <Uron/render/Renderer.h>
#include <Uron/Logger.h>

namespace Uron {

Scene2D::Scene2D() : m_name("Scene") {
    m_root = std::make_unique<Node2D>("root");
}

Scene2D::Scene2D(const std::string& name) : m_name(name) {
    m_root = std::make_unique<Node2D>("root");
}

Scene2D::~Scene2D() = default;

void Scene2D::enter() {
    if (m_active) return;
    m_active = true;
    if (m_root) m_root->enterTree(*this);
    URON_INFO("Scene2D activa: " + m_name);
}

void Scene2D::exit() {
    if (!m_active) return;
    if (m_root) m_root->exitTree(*this);
    m_active = false;
    URON_INFO("Scene2D desactivada: " + m_name);
}

void Scene2D::update(float dt) {
    if (!m_active || !m_root) return;
    m_root->updateTree(*this, dt);
}

void Scene2D::render(Renderer& renderer) {
    if (!m_active || !m_root) return;
    Mat4 view = m_camera ? m_camera->viewMatrix() : Mat4::identity();
    m_root->renderTree(renderer, view, Color::White());
}

}