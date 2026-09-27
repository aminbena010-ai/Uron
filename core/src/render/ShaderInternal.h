#pragma once
// ---------------------------------------------------------------------------
//  render/ShaderInternal.h  (PRIVADO: no sale del motor)
//  Estado CPU de un Shader compartido entre Shader.cpp (propietario) y
//  VulkanRenderer.cpp (consumidor via Shader::internal()). Sin tipos Vulkan.
//
//  Layout de push constants para shaders custom (CLAUDE.md §5):
//     offset  0: vec2 position      (sprite)
//     offset  8: vec2 size          (sprite)
//     offset 16: vec2 screenSize    (sprite)
//     offset 24: uniforms (std430, en el orden de la primera setUniform)
//     total    : 128 bytes (32 floats)
// ---------------------------------------------------------------------------
#include <Uron/render/Shader.h>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace Uron {

enum class UniformType {
    Float,
    Int,
    Vec2,
    Vec3,
    Vec4,
    Mat4
};

struct UniformValue {
    UniformType type = UniformType::Float;
    union {
        f32 f;
        i32 i;
        f32 v2[2];
        f32 v3[3];
        f32 v4[4];
        f32 m4[16];
    };
    UniformValue() { std::memset(&m4, 0, sizeof(m4)); }
};

constexpr u32 SPRITE_PUSH_BYTES  = sizeof(float) * 6;    // position/size/screenSize
constexpr u32 CUSTOM_PUSH_BYTES  = sizeof(float) * 32;   // total (regla CLAUDE.md §5)
constexpr u32 UNIFORM_PUSH_BYTES = CUSTOM_PUSH_BYTES - SPRITE_PUSH_BYTES;

struct ShaderData {
    u64 handle = 0;
    ShaderDesc desc;
    bool loaded = false;
    std::string vertPath;
    std::string fragPath;

    std::unordered_map<std::string, UniformValue> uniforms;
    std::vector<std::string> uniformOrder;

    // dst = bloque completo de push constants (128 bytes) con los 6 floats
    // del sprite ya escritos en [0,24). Rellena [24,maxBytes) con los
    // uniforms (alineacion std430 respecto al offset absoluto) y devuelve
    // el offset final absoluto.
    u32 packUniforms(void* dst, u32 maxBytes) const;

    mutable bool warnedOverflow = false;
};

}
