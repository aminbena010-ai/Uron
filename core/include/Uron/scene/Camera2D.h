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

    // Matriz mundo→pantalla (pixeles, origen arriba-izquierda): centra la
    // posición de la cámara en el centro del viewport y aplica zoom/rotación.
    // Con la cámara en (viewW/2, viewH/2), zoom 1 y rotación 0 => identidad.
    // El Renderer la consume en Scene2D::render; no genera NDC.
    Mat4 viewMatrix() const;

private:
    f32 m_zoom  = 1.f;
    f32 m_viewW = 1280.f;
    f32 m_viewH = 720.f;
};

inline Camera2D::Camera2D() : Node2D("Camera2D") {}
inline Camera2D::Camera2D(const std::string& name) : Node2D(name) {}

inline Mat4 Camera2D::viewMatrix() const {
    Mat4 center = Mat4::translation({m_viewW * 0.5f, m_viewH * 0.5f, 0.f});
    Mat4 zoom   = Mat4::scale({m_zoom, m_zoom, 1.f});
    Mat4 rot    = Mat4::rotationZ(-m_rotation);
    Mat4 move   = Mat4::translation({-m_position.x, -m_position.y, 0.f});
    return center * zoom * rot * move;
}

}