#pragma once
#include <Uron/math/Vec3.h>
#include <Uron/math/Vec4.h>
#include <cmath>

namespace Uron {

struct Mat4 {
    f32 m[4][4] = {
        {1,0,0,0},
        {0,1,0,0},
        {0,0,1,0},
        {0,0,0,1}
    };

    Mat4() = default;

    static Mat4 identity() { return Mat4{}; }

    static Mat4 translation(const Vec3& t) {
        Mat4 r;
        r.m[3][0] = t.x;
        r.m[3][1] = t.y;
        r.m[3][2] = t.z;
        return r;
    }

    static Mat4 scale(const Vec3& s) {
        Mat4 r;
        r.m[0][0] = s.x;
        r.m[1][1] = s.y;
        r.m[2][2] = s.z;
        return r;
    }

    static Mat4 rotationX(f32 rad) {
        Mat4 r;
        f32 c = std::cos(rad), s = std::sin(rad);
        r.m[1][1] =  c; r.m[1][2] =  s;
        r.m[2][1] = -s; r.m[2][2] =  c;
        return r;
    }

    static Mat4 rotationY(f32 rad) {
        Mat4 r;
        f32 c = std::cos(rad), s = std::sin(rad);
        r.m[0][0] =  c; r.m[0][2] = -s;
        r.m[2][0] =  s; r.m[2][2] =  c;
        return r;
    }

    static Mat4 rotationZ(f32 rad) {
        Mat4 r;
        f32 c = std::cos(rad), s = std::sin(rad);
        r.m[0][0] =  c; r.m[0][1] =  s;
        r.m[1][0] = -s; r.m[1][1] =  c;
        return r;
    }

    static Mat4 perspective(f32 fovY, f32 aspect, f32 nearZ, f32 farZ) {
        Mat4 r;
        f32 tanHalf = std::tan(fovY * 0.5f);
        r.m[0][0] = 1.f / (aspect * tanHalf);
        r.m[1][1] = -1.f / tanHalf;
        r.m[2][2] = farZ / (nearZ - farZ);
        r.m[2][3] = -1.f;
        r.m[3][2] = (farZ * nearZ) / (nearZ - farZ);
        r.m[3][3] = 0.f;
        return r;
    }

    static Mat4 ortho(f32 l, f32 r_, f32 b, f32 t, f32 n, f32 f) {
        Mat4 r;
        r.m[0][0] = 2.f / (r_ - l);
        r.m[1][1] = 2.f / (b - t);
        r.m[2][2] = 1.f / (n - f);
        r.m[3][0] = (r_ + l) / (l - r_);
        r.m[3][1] = (t + b) / (b - t);
        r.m[3][2] = n / (n - f);
        return r;
    }

    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = (center - eye).normalized();
        Vec3 s = f.cross(up).normalized();
        Vec3 u = s.cross(f);

        Mat4 r;
        r.m[0][0] =  s.x; r.m[0][1] =  u.x; r.m[0][2] = -f.x;
        r.m[1][0] =  s.y; r.m[1][1] =  u.y; r.m[1][2] = -f.y;
        r.m[2][0] =  s.z; r.m[2][1] =  u.z; r.m[2][2] = -f.z;
        r.m[3][0] = -s.dot(eye);
        r.m[3][1] = -u.dot(eye);
        r.m[3][2] =  f.dot(eye);
        return r;
    }

    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int c = 0; c < 4; ++c)
            for (int row = 0; row < 4; ++row) {
                r.m[c][row] = 0.f;
                for (int k = 0; k < 4; ++k)
                    r.m[c][row] += m[k][row] * o.m[c][k];
            }
        return r;
    }

    Vec4 operator*(const Vec4& v) const {
        return {
            m[0][0]*v.x + m[1][0]*v.y + m[2][0]*v.z + m[3][0]*v.w,
            m[0][1]*v.x + m[1][1]*v.y + m[2][1]*v.z + m[3][1]*v.w,
            m[0][2]*v.x + m[1][2]*v.y + m[2][2]*v.z + m[3][2]*v.w,
            m[0][3]*v.x + m[1][3]*v.y + m[2][3]*v.z + m[3][3]*v.w
        };
    }

    Mat4 transposed() const {
        Mat4 r;
        for (int c = 0; c < 4; ++c)
            for (int row = 0; row < 4; ++row)
                r.m[c][row] = m[row][c];
        return r;
    }

    static Mat4 rotate(f32 rad, const Vec3& axis) {
        Vec3 a = axis.normalized();
        f32 c = std::cos(rad);
        f32 s = std::sin(rad);
        f32 t = 1.f - c;

        Mat4 r;
        r.m[0][0] = t*a.x*a.x + c;
        r.m[0][1] = t*a.x*a.y + s*a.z;
        r.m[0][2] = t*a.x*a.z - s*a.y;
        r.m[1][0] = t*a.x*a.y - s*a.z;
        r.m[1][1] = t*a.y*a.y + c;
        r.m[1][2] = t*a.y*a.z + s*a.x;
        r.m[2][0] = t*a.x*a.z + s*a.y;
        r.m[2][1] = t*a.y*a.z - s*a.x;
        r.m[2][2] = t*a.z*a.z + c;
        return r;
    }
};

}