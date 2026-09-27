#pragma once
#include <Uron/scene/Node2D.h>
#include <Uron/math/Vec2.h>
#include <Uron/render/Texture.h>

namespace Uron {

class Sprite2D : public Node2D {
public:
    Sprite2D();
    explicit Sprite2D(const std::string& name);
    ~Sprite2D() override;

    Vec2 size() const { return m_size; }
    void setSize(const Vec2& s) { m_size = s; }

    void setTexture(const std::string& path);
    Texture& texture() { return m_texture; }

    void onRender(Renderer& renderer) override;

private:
    Vec2 m_size{1.f, 1.f};
    Texture m_texture;
};

}