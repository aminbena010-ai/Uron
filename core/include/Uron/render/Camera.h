#pragma once
#include <Uron/math/Vec3.h>
#include <Uron/math/Mat4.h>

namespace Uron {

struct Camera3D {
    Vec3  position = {0.f, 0.f, 5.f};
    Vec3  target   = {0.f, 0.f, 0.f};
    Vec3  up       = {0.f, 1.f, 0.f};
    f32   fovY     = 1.0472f;
    f32   aspect   = 16.f / 9.f;
    f32   nearZ    = 0.1f;
    f32   farZ     = 1000.f;

    Mat4 viewMatrix() const {
        return Mat4::lookAt(position, target, up);
    }

    Mat4 projectionMatrix() const {
        return Mat4::perspective(fovY, aspect, nearZ, farZ);
    }

    Mat4 viewProjection() const {
        return projectionMatrix() * viewMatrix();
    }
};

}