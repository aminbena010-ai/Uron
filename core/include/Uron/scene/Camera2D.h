#pragma once
#include <Uron/scene/Node2D.h>
#include <Uron/math/Mat4.h>

namespace Uron {

class Camera2D : public Node2D {
public:
    Camera2D();
    explicit Camera2D(const std::string& name);

    f32  zoom()    const { return m_zoom; }
    void setZoom(f32 z) { m_zoom = z; }

    void setViewportSize(f32 w, f32 h) { m_viewW = w; m_viewH = h; }

    Mat4 viewProjection() const;

private:
    f32 m_zoom  = 1.f;
    f32 m_viewW = 1280.f;
    f32 m_viewH = 720.f;
};

inline Camera2D::Camera2D() : Node2D("Camera2D") {}
inline Camera2D::Camera2D(const std::string& name) : Node2D(name) {}

inline Mat4 Camera2D::viewProjection() const {
    Mat4 t = Mat4::translation({-m_position.x, -m_position.y, 0.f});
    Mat4 r = Mat4::rotationZ(-m_rotation);
    Mat4 s = Mat4::scale({m_zoom, m_zoom, 1.f});

    f32 halfW = m_viewW * 0.5f;
    f32 halfH = m_viewH * 0.5f;
    Mat4 p = Mat4::ortho(-halfW, halfW, -halfH, halfH, -1.f, 1.f);

    return p * s * r * t;
}

}