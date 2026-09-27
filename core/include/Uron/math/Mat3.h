// ============================================================================
//  math/Mat3.h
//  ---------------------------------------------------------------------------
//  QUE ES: Matriz 3x3.
//  CONTIENE: struct Mat3 con identidad, multiplicacion, transpose,
//            determinante, inversa.
//  PARA QUE: Rotaciones 2D, normales, transformaciones sin traslacion.
//  QUIEN LO USA: Render 2D, transformaciones de normales.
//  NOTA: Columna-mayor (como OpenGL/Vulkan). m[col][fila].
// ============================================================================
#pragma once
#include <Uron/math/Vec3.h>

namespace Uron {

struct Mat3 {
    f32 m[3][3] = {
        {1,0,0},
        {0,1,0},
        {0,0,1}
    };

    Mat3() = default;

    static constexpr Mat3 identity() { return Mat3{}; }

    // No es constexpr: std::cos/std::sin no lo son en C++17.
    static Mat3 rotation(f32 radians) {
        Mat3 r;
        f32 c = std::cos(radians);
        f32 s = std::sin(radians);
        r.m[0][0] =  c; r.m[0][1] =  s; r.m[0][2] = 0;
        r.m[1][0] = -s; r.m[1][1] =  c; r.m[1][2] = 0;
        r.m[2][0] =  0; r.m[2][1] =  0; r.m[2][2] = 1;
        return r;
    }

    static Mat3 scale(const Vec3& s) {
        Mat3 r;
        r.m[0][0] = s.x; r.m[1][1] = s.y; r.m[2][2] = s.z;
        return r;
    }

    Mat3 operator*(const Mat3& o) const {
        Mat3 r;
        for (int c = 0; c < 3; ++c)
            for (int row = 0; row < 3; ++row) {
                r.m[c][row] = 0.f;
                for (int k = 0; k < 3; ++k)
                    r.m[c][row] += m[k][row] * o.m[c][k];
            }
        return r;
    }

    Vec3 operator*(const Vec3& v) const {
        return {
            m[0][0]*v.x + m[1][0]*v.y + m[2][0]*v.z,
            m[0][1]*v.x + m[1][1]*v.y + m[2][1]*v.z,
            m[0][2]*v.x + m[1][2]*v.y + m[2][2]*v.z
        };
    }

    Mat3 transposed() const {
        Mat3 r;
        for (int c = 0; c < 3; ++c)
            for (int row = 0; row < 3; ++row)
                r.m[c][row] = m[row][c];
        return r;
    }

    f32 determinant() const {
        return m[0][0]*(m[1][1]*m[2][2] - m[2][1]*m[1][2])
             - m[1][0]*(m[0][1]*m[2][2] - m[2][1]*m[0][2])
             + m[2][0]*(m[0][1]*m[1][2] - m[1][1]*m[0][2]);
    }

    Mat3 inversed() const {
        f32 det = determinant();
        if (det == 0.f) return Mat3{};
        f32 inv = 1.f / det;
        Mat3 r;
        r.m[0][0] =  (m[1][1]*m[2][2] - m[2][1]*m[1][2]) * inv;
        r.m[0][1] = -(m[0][1]*m[2][2] - m[2][1]*m[0][2]) * inv;
        r.m[0][2] =  (m[0][1]*m[1][2] - m[1][1]*m[0][2]) * inv;
        r.m[1][0] = -(m[1][0]*m[2][2] - m[2][0]*m[1][2]) * inv;
        r.m[1][1] =  (m[0][0]*m[2][2] - m[2][0]*m[0][2]) * inv;
        r.m[1][2] = -(m[0][0]*m[1][2] - m[1][0]*m[0][2]) * inv;
        r.m[2][0] =  (m[1][0]*m[2][1] - m[2][0]*m[1][1]) * inv;
        r.m[2][1] = -(m[0][0]*m[2][1] - m[2][0]*m[0][1]) * inv;
        r.m[2][2] =  (m[0][0]*m[1][1] - m[1][0]*m[0][1]) * inv;
        return r;
    }
};

}