// ============================================================================
//  math/Quat.h
//  ---------------------------------------------------------------------------
//  QUE ES: Cuaternion (rotacion 3D sin gimbal lock).
//  CONTIENE: struct Quat con identidad, fromAxisAngle, fromEuler,
//            multiplicacion, normalize, toMat4, slerp.
//  PARA QUE: Rotaciones 3D suaves, interpolacion (slerp), animaciones.
//  QUIEN LO USA: Render 3D, animacion, camaras, fisicas.
//  EJEMPLO:
//     Quat q = Quat::fromAxisAngle(Vec3::up(), 1.57f);
//     Mat4 rot = q.toMat4();
// ============================================================================
#pragma once
#include <Uron/math/Vec3.h>
#include <Uron/math/Mat4.h>
#include <cmath>

namespace Uron {

struct Quat {
    f32 x = 0.f;
    f32 y = 0.f;
    f32 z = 0.f;
    f32 w = 1.f;

    Quat() = default;
    Quat(f32 x_, f32 y_, f32 z_, f32 w_) : x(x_), y(y_), z(z_), w(w_) {}

    static Quat identity() { return {0, 0, 0, 1}; }

    static Quat fromAxisAngle(const Vec3& axis, f32 rad) {
        Vec3 a = axis.normalized();
        f32 half = rad * 0.5f;
        f32 s = std::sin(half);
        return {a.x * s, a.y * s, a.z * s, std::cos(half)};
    }

    static Quat fromEuler(f32 pitch, f32 yaw, f32 roll) {
        f32 cy = std::cos(yaw * 0.5f);
        f32 sy = std::sin(yaw * 0.5f);
        f32 cp = std::cos(pitch * 0.5f);
        f32 sp = std::sin(pitch * 0.5f);
        f32 cr = std::cos(roll * 0.5f);
        f32 sr = std::sin(roll * 0.5f);

        return {
            cy * sp * cr + sy * cp * sr,
            sy * cp * cr - cy * sp * sr,
            cy * cp * sr - sy * sp * cr,
            cy * cp * cr + sy * sp * sr
        };
    }

    Quat operator*(const Quat& o) const {
        return {
            w*o.x + x*o.w + y*o.z - z*o.y,
            w*o.y - x*o.z + y*o.w + z*o.x,
            w*o.z + x*o.y - y*o.x + z*o.w,
            w*o.w - x*o.x - y*o.y - z*o.z
        };
    }

    f32 lengthSq() const { return x*x + y*y + z*z + w*w; }
    f32 length()   const { return std::sqrt(lengthSq()); }

    Quat normalized() const {
        f32 len = length();
        if (len == 0.f) return identity();
        f32 inv = 1.f / len;
        return {x*inv, y*inv, z*inv, w*inv};
    }

    void normalize() {
        f32 len = length();
        if (len == 0.f) return;
        f32 inv = 1.f / len;
        x *= inv; y *= inv; z *= inv; w *= inv;
    }

    Quat conjugated() const { return {-x, -y, -z, w}; }

    Mat4 toMat4() const {
        Mat4 r;
        f32 xx = x*x, yy = y*y, zz = z*z;
        f32 xy = x*y, xz = x*z, yz = y*z;
        f32 wx = w*x, wy = w*y, wz = w*z;

        r.m[0][0] = 1.f - 2.f*(yy + zz);
        r.m[0][1] = 2.f*(xy + wz);
        r.m[0][2] = 2.f*(xz - wy);

        r.m[1][0] = 2.f*(xy - wz);
        r.m[1][1] = 1.f - 2.f*(xx + zz);
        r.m[1][2] = 2.f*(yz + wx);

        r.m[2][0] = 2.f*(xz + wy);
        r.m[2][1] = 2.f*(yz - wx);
        r.m[2][2] = 1.f - 2.f*(xx + yy);
        return r;
    }

    static Quat slerp(const Quat& a, const Quat& b, f32 t) {
        f32 dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
        Quat bb = b;
        if (dot < 0.f) { bb = {-b.x, -b.y, -b.z, -b.w}; dot = -dot; }

        if (dot > 0.9995f) {
            return Quat{
                a.x + t*(bb.x - a.x),
                a.y + t*(bb.y - a.y),
                a.z + t*(bb.z - a.z),
                a.w + t*(bb.w - a.w)
            }.normalized();
        }

        f32 theta0 = std::acos(dot);
        f32 theta  = theta0 * t;
        f32 sinT0  = std::sin(theta0);
        f32 sinT   = std::sin(theta);

        f32 s0 = std::cos(theta) - dot * sinT / sinT0;
        f32 s1 = sinT / sinT0;

        return Quat{
            a.x*s0 + bb.x*s1,
            a.y*s0 + bb.y*s1,
            a.z*s0 + bb.z*s1,
            a.w*s0 + bb.w*s1
        };
    }
};

}