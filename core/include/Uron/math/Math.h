// ============================================================================
//  math/Math.h
//  ---------------------------------------------------------------------------
//  QUE ES: Utilidades matematicas generales.
//  CONTIENE: constantes (PI, TAU, EPSILON), funciones (radians, degrees,
//            clamp, lerp, min, max, abs, sqrt, pow, sin, cos, tan).
//  PARA QUE: Que el motor y el usuario tengan funciones matematicas
//            comunes sin depender de <cmath> directamente.
//  QUIEN LO USA: Todo el motor, plugins, usuario.
//  EJEMPLO:
//     float r = Math::radians(90.f);
//     float c = Math::clamp(x, 0.f, 1.f);
//     float l = Math::lerp(a, b, 0.5f);
// ============================================================================
#pragma once
#include <Uron/Types.h>
#include <cmath>
#include <algorithm>

namespace Uron {

namespace Math {

inline constexpr f32 PI      = 3.14159265358979323846f;
inline constexpr f32 TAU     = 6.28318530717958647692f;
inline constexpr f32 HALF_PI = 1.57079632679489661923f;
inline constexpr f32 EPSILON = 1e-6f;
inline constexpr f32 INF     = 1e30f;

inline f32 radians(f32 deg) { return deg * (PI / 180.f); }
inline f32 degrees(f32 rad) { return rad * (180.f / PI); }

template<typename T>
inline T min(T a, T b) { return a < b ? a : b; }

template<typename T>
inline T max(T a, T b) { return a > b ? a : b; }

template<typename T>
inline T clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

template<typename T>
inline T lerp(T a, T b, f32 t) { return a + (b - a) * t; }

inline f32 abs(f32 v)  { return v < 0.f ? -v : v; }
inline f32 sqrt(f32 v) { return std::sqrt(v); }
inline f32 pow(f32 b, f32 e) { return std::pow(b, e); }
inline f32 sin(f32 v)  { return std::sin(v); }
inline f32 cos(f32 v)  { return std::cos(v); }
inline f32 tan(f32 v)  { return std::tan(v); }
inline f32 asin(f32 v) { return std::asin(v); }
inline f32 acos(f32 v) { return std::acos(v); }
inline f32 atan2(f32 y, f32 x) { return std::atan2(y, x); }
inline f32 floor(f32 v) { return std::floor(v); }
inline f32 ceil(f32 v)  { return std::ceil(v); }
inline f32 round(f32 v) { return std::round(v); }

inline bool approx(f32 a, f32 b, f32 eps = EPSILON) {
    return abs(a - b) < eps;
}

}

}