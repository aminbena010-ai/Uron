#pragma once
#include <Uron/Types.h>

namespace Uron {

struct Color {
    f32 r = 0.f;
    f32 g = 0.f;
    f32 b = 0.f;
    f32 a = 1.f;

    constexpr Color() = default;
    constexpr Color(f32 r_, f32 g_, f32 b_, f32 a_ = 1.f)
        : r(r_), g(g_), b(b_), a(a_) {}

    static constexpr Color fromHex(u32 hex, f32 alpha = 1.f) {
        return {
            static_cast<f32>((hex >> 16) & 0xFF) / 255.f,
            static_cast<f32>((hex >>  8) & 0xFF) / 255.f,
            static_cast<f32>( hex        & 0xFF) / 255.f,
            alpha
        };
    }

    u32 toHex() const {
        u32 ri = static_cast<u32>(r * 255.f) & 0xFF;
        u32 gi = static_cast<u32>(g * 255.f) & 0xFF;
        u32 bi = static_cast<u32>(b * 255.f) & 0xFF;
        return (ri << 16) | (gi << 8) | bi;
    }

    static constexpr Color Black()   { return {0.f, 0.f, 0.f, 1.f}; }
    static constexpr Color White()   { return {1.f, 1.f, 1.f, 1.f}; }
    static constexpr Color Red()     { return {1.f, 0.f, 0.f, 1.f}; }
    static constexpr Color Green()   { return {0.f, 1.f, 0.f, 1.f}; }
    static constexpr Color Blue()    { return {0.f, 0.f, 1.f, 1.f}; }
    static constexpr Color Yellow()  { return {1.f, 1.f, 0.f, 1.f}; }
    static constexpr Color Cyan()    { return {0.f, 1.f, 1.f, 1.f}; }
    static constexpr Color Magenta() { return {1.f, 0.f, 1.f, 1.f}; }
    static constexpr Color Gray()    { return {0.5f, 0.5f, 0.5f, 1.f}; }
    static constexpr Color Clear()   { return {0.f, 0.f, 0.f, 0.f}; }
};

}