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
};

using Handle = u64;
constexpr Handle InvalidHandle = 0;

}