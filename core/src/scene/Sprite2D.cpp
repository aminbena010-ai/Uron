#include <Uron/scene/Sprite2D.h>
#include <Uron/render/Renderer.h>
#include <Uron/render/Texture.h>
#include <Uron/render/Shader.h>

namespace Uron {

Sprite2D::Sprite2D() : Node2D("Sprite") {}
Sprite2D::Sprite2D(const std::string& name) : Node2D(name) {}
Sprite2D::~Sprite2D() = default;

bool Sprite2D::setTexture(const std::string& path) {
    return m_texture.loadFromFile(path);
}

void Sprite2D::onRender(Renderer& renderer) {
    if (!m_texture.isValid()) return;

    // El quad unitario [0,1]² se escala por m_size y hereda la transform
    // mundial (padres, rotación, escala) ya compuesta por renderTree.
    Mat4 quad = m_world * Mat4::scale({m_size.x, m_size.y, 1.f});

    renderer.drawSprite(m_texture, quad, m_shader, m_worldTint);
}

}