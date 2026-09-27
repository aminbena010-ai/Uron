#pragma once
#include <Uron/Types.h>
#include <cmath>

namespace Uron {

struct Vec3 {
    f32 x = 0.f;
    f32 y = 0.f;
    f32 z = 0.f;

    constexpr Vec3() = default;
    constexpr Vec3(f32 x_, f32 y_, f32 z_) : x(x_), y(y_), z(z_) {}
    constexpr explicit Vec3(f32 s) : x(s), y(s), z(s) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(f32 s)          const { return {x * s,   y * s,   z * s};   }
    Vec3 operator/(f32 s)          const { return {x / s,   y / s,   z / s};   }
    Vec3 operator-()               const { return {-x, -y, -z};                }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(f32 s)         { x *= s;   y *= s;   z *= s;   return *this; }
    Vec3& operator/=(f32 s)         { x /= s;   y /= s;   z /= s;   return *this; }

    bool operator==(const Vec3& o) const { return x == o.x && y == o.y && z == o.z; }
    bool operator!=(const Vec3& o) const { return !(*this == o); }

    f32 dot(const Vec3& o)   const { return x * o.x + y * o.y + z * o.z; }
    f32 lengthSq()           const { return x * x + y * y + z * z; }
    f32 length()             const { return std::sqrt(lengthSq()); }

    Vec3 cross(const Vec3& o) const {
        return {
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        };
    }

    Vec3 normalized() const {
        f32 len = length();
        return len > 0.f ? Vec3{x / len, y / len, z / len} : Vec3{};
    }

    void normalize() {
        f32 len = length();
        if (len > 0.f) { x /= len; y /= len; z /= len; }
    }

    static Vec3 zero()  { return {0.f, 0.f, 0.f}; }
    static Vec3 one()   { return {1.f, 1.f, 1.f}; }
    static Vec3 up()    { return {0.f, 1.f, 0.f}; }
    static Vec3 down()  { return {0.f,-1.f, 0.f}; }
    static Vec3 right() { return {1.f, 0.f, 0.f}; }
    static Vec3 left()  { return {-1.f,0.f, 0.f}; }
    static Vec3 forward(){ return {0.f, 0.f, 1.f}; }
    static Vec3 back()  { return {0.f, 0.f,-1.f}; }
};

inline Vec3 operator*(f32 s, const Vec3& v) { return {v.x * s, v.y * s, v.z * s}; }

}