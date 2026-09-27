#pragma once
#include <Uron/Types.h>
#include <cmath>

namespace Uron {

struct Vec2 {
    f32 x = 0.f;
    f32 y = 0.f;

    constexpr Vec2() = default;
    constexpr Vec2(f32 x_, f32 y_) : x(x_), y(y_) {}
    constexpr explicit Vec2(f32 s) : x(s), y(s) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(f32 s)          const { return {x * s,   y * s};   }
    Vec2 operator/(f32 s)          const { return {x / s,   y / s};   }
    Vec2 operator-()               const { return {-x, -y};           }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(f32 s)         { x *= s;   y *= s;   return *this; }
    Vec2& operator/=(f32 s)         { x /= s;   y /= s;   return *this; }

    bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vec2& o) const { return !(*this == o); }

    f32 dot(const Vec2& o)   const { return x * o.x + y * o.y; }
    f32 cross(const Vec2& o) const { return x * o.y - y * o.x; }
    f32 lengthSq()           const { return x * x + y * y; }
    f32 length()             const { return std::sqrt(lengthSq()); }

    Vec2 normalized() const {
        f32 len = length();
        return len > 0.f ? Vec2{x / len, y / len} : Vec2{};
    }

    void normalize() {
        f32 len = length();
        if (len > 0.f) { x /= len; y /= len; }
    }

    static constexpr Vec2 zero()  { return {0.f, 0.f}; }
    static constexpr Vec2 one()   { return {1.f, 1.f}; }
    static Vec2 up()    { return {0.f, 1.f}; }
    static Vec2 down()  { return {0.f,-1.f}; }
    static Vec2 left()  { return {-1.f,0.f}; }
    static Vec2 right() { return {1.f, 0.f}; }
};

inline Vec2 operator*(f32 s, const Vec2& v) { return {v.x * s, v.y * s}; }

}