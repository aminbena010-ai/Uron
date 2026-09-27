#include <Uron/scene/Sprite2D.h>
#include <Uron/render/Renderer.h>
#include <Uron/render/Texture.h>
#include <Uron/Logger.h>

namespace Uron {

Sprite2D::Sprite2D() : Node2D("Sprite") {}
Sprite2D::Sprite2D(const std::string& name) : Node2D(name) {}
Sprite2D::~Sprite2D() = default;

void Sprite2D::setTexture(const std::string& path) {
    m_texture.loadFromFile(path);
}

void Sprite2D::onRender(Renderer& renderer) {
    static int calls = 0;
    if (calls < 3) {
        URON_INFO("Sprite2D::onRender (" + m_name + ")");
        calls++;
    }

    if (!m_texture.isValid()) {
        URON_WARN("Sprite2D: textura invalida, no se dibuja");
        return;
    }

    Mat4 t = Mat4::identity();
    t.m[0][0] = m_size.x;
    t.m[1][1] = m_size.y;
    t.m[3][0] = m_position.x;
    t.m[3][1] = m_position.y;

    renderer.drawSprite(m_texture, t);
}

}