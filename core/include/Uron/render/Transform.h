// ============================================================================
//  render/Transform.h
//  ---------------------------------------------------------------------------
//  QUE ES: Transformacion (posicion + rotacion + escala).
//  CONTIENE: struct Transform con position, rotation, scale, y matrix().
//  PARA QUE: Que el usuario mueva/rote/escale objetos sin tocar matrices.
//  QUIEN LO USA: El usuario (objetos del juego), el motor, plugins.
//  EJEMPLO:
//     Transform t;
//     t.position = {100.f, 50.f, 0.f};
//     t.scale    = {2.f, 2.f, 2.f};
//     Mat4 m = t.matrix();
// ============================================================================
#pragma once
#include <Uron/math/Vec3.h>
#include <Uron/math/Mat4.h>

namespace Uron {

struct Transform {
    Vec3 position = {0.f, 0.f, 0.f};
    Vec3 rotation = {0.f, 0.f, 0.f};   // Euler en radianes
    Vec3 scale    = {1.f, 1.f, 1.f};

    Mat4 matrix() const {
        Mat4 t = Mat4::translation(position);
        Mat4 r = Mat4::rotationZ(rotation.z)
               * Mat4::rotationY(rotation.y)
               * Mat4::rotationX(rotation.x);
        Mat4 s = Mat4::scale(scale);
        return t * r * s;
    }

    void translate(const Vec3& d) { position += d; }
    void rotate(const Vec3& d)    { rotation += d; }
};

}