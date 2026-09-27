#pragma once
#include <cstdint>

namespace Uron {

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

struct Rect {
    f32 x = 0.f, y = 0.f;
    f32 w = 0.f, h = 0.f;

    Rect() = default;
    Rect(f32 x_, f32 y_, f32 w_, f32 h_)
        : x(x_), y(y_), w(w_), h(h_) {}

    constexpr f32 right()  const { return x + w; }
    constexpr f32 bottom() const { return y + h; }
    constexpr f32 centerX() const { return x + w * 0.5f; }
    constexpr f32 centerY() const { return y + h * 0.5f; }

    constexpr bool contains(f32 px, f32 py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    constexpr bool intersects(const Rect& o) const {
        return x < o.right() && right() > o.x &&
               y < o.bottom() && bottom() > o.y;
    }
};

// Handle con indizacion + generacion: la generacion evita usar referencias
// invalidas tras free+realloc del slot (ABA). index 0 / generation 0 =
// invalido (los slots validos arrancan en generacion 1).
struct Handle {
    u32 index      = 0;
    u32 generation = 0;

    constexpr bool operator==(const Handle& o) const {
        return index == o.index && generation == o.generation;
    }
    constexpr bool operator!=(const Handle& o) const { return !(*this == o); }
    constexpr bool valid() const { return generation != 0; }
};

constexpr Handle InvalidHandle{0, 0};

}