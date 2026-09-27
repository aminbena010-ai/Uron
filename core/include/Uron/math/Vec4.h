#pragma once
#include <Uron/Types.h>
#include <cmath>

namespace Uron {

struct Vec4 {
    f32 x = 0.f;
    f32 y = 0.f;
    f32 z = 0.f;
    f32 w = 0.f;

    constexpr Vec4() = default;
    constexpr Vec4(f32 x_, f32 y_, f32 z_, f32 w_)
        : x(x_), y(y_), z(z_), w(w_) {}
    constexpr explicit Vec4(f32 s) : x(s), y(s), z(s), w(s) {}

    Vec4 operator+(const Vec4& o) const { return {x+o.x, y+o.y, z+o.z, w+o.w}; }
    Vec4 operator-(const Vec4& o) const { return {x-o.x, y-o.y, z-o.z, w-o.w}; }
    Vec4 operator*(f32 s)          const { return {x*s,   y*s,   z*s,   w*s};   }
    Vec4 operator/(f32 s)          const { return {x/s,   y/s,   z/s,   w/s};   }
    Vec4 operator-()               const { return {-x, -y, -z, -w};             }

    Vec4& operator+=(const Vec4& o) { x+=o.x; y+=o.y; z+=o.z; w+=o.w; return *this; }
    Vec4& operator-=(const Vec4& o) { x-=o.x; y-=o.y; z-=o.z; w-=o.w; return *this; }
    Vec4& operator*=(f32 s)         { x*=s;   y*=s;   z*=s;   w*=s;   return *this; }
    Vec4& operator/=(f32 s)         { x/=s;   y/=s;   z/=s;   w/=s;   return *this; }

    bool operator==(const Vec4& o) const { return x==o.x && y==o.y && z==o.z && w==o.w; }
    bool operator!=(const Vec4& o) const { return !(*this == o); }

    f32 dot(const Vec4& o)  const { return x*o.x + y*o.y + z*o.z + w*o.w; }
    f32 lengthSq()          const { return x*x + y*y + z*z + w*w; }
    f32 length()            const { return std::sqrt(lengthSq()); }

    Vec4 normalized() const {
        f32 len = length();
        return len > 0.f ? Vec4{x/len, y/len, z/len, w/len} : Vec4{};
    }

    static Vec4 zero() { return {0.f, 0.f, 0.f, 0.f}; }
    static Vec4 one()  { return {1.f, 1.f, 1.f, 1.f}; }
};

inline Vec4 operator*(f32 s, const Vec4& v) { return {v.x*s, v.y*s, v.z*s, v.w*s}; }

}