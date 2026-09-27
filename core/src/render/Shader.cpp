#include <Uron/render/Shader.h>
#include "render/ShaderInternal.h"
#include <Uron/Logger.h>

#include <atomic>
#include <cstring>
#include <fstream>

namespace Uron {

namespace {

std::atomic<u64> s_nextHandle{1};

bool fileReadable(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return f.good();
}

UniformValue& fetchUniform(ShaderData& d, const char* name) {
    auto it = d.uniforms.find(name);
    if (it == d.uniforms.end()) {
        d.uniformOrder.emplace_back(name);
        it = d.uniforms.emplace(name, UniformValue{}).first;
    }
    return it->second;
}

}

struct Shader::Impl {
    ShaderData data;
};

Shader::Shader() : impl(new Impl) {}

Shader::~Shader() {
    destroy();
    delete impl;
}

Shader::Shader(Shader&& o) noexcept : impl(o.impl) {
    o.impl = nullptr;
}

Shader& Shader::operator=(Shader&& o) noexcept {
    if (this != &o) {
        destroy();
        delete impl;
        impl = o.impl;
        o.impl = nullptr;
    }
    return *this;
}

bool Shader::loadFromFile(const std::string& vertPath,
                          const std::string& fragPath,
                          const ShaderDesc& desc) {
    if (!impl) return false;

    if (!fileReadable(vertPath)) {
        URON_ERROR("Shader: no se pudo leer " + vertPath);
        return false;
    }
    if (!fileReadable(fragPath)) {
        URON_ERROR("Shader: no se pudo leer " + fragPath);
        return false;
    }

    ShaderData& d = impl->data;
    d.vertPath = vertPath;
    d.fragPath = fragPath;
    d.desc = desc;
    d.uniforms.clear();
    d.uniformOrder.clear();
    d.handle = s_nextHandle.fetch_add(1);
    d.loaded = true;

    URON_INFO("Shader cargado: " + vertPath + " + " + fragPath);
    return true;
}

void Shader::destroy() {
    if (!impl) return;
    impl->data = ShaderData{};
}

void Shader::bind() {}
void Shader::unbind() {}

bool Shader::isValid() const {
    return impl && impl->data.loaded && impl->data.handle != 0;
}

void Shader::setUniform(const char* name, f32 v) {
    if (!impl || !name) return;
    UniformValue& u = fetchUniform(impl->data, name);
    u.type = UniformType::Float;
    u.f = v;
}

void Shader::setUniform(const char* name, i32 v) {
    if (!impl || !name) return;
    UniformValue& u = fetchUniform(impl->data, name);
    u.type = UniformType::Int;
    u.i = v;
}

void Shader::setUniform(const char* name, const Vec2& v) {
    if (!impl || !name) return;
    UniformValue& u = fetchUniform(impl->data, name);
    u.type = UniformType::Vec2;
    u.v2[0] = v.x; u.v2[1] = v.y;
}

void Shader::setUniform(const char* name, const Vec3& v) {
    if (!impl || !name) return;
    UniformValue& u = fetchUniform(impl->data, name);
    u.type = UniformType::Vec3;
    u.v3[0] = v.x; u.v3[1] = v.y; u.v3[2] = v.z;
}

void Shader::setUniform(const char* name, const Vec4& v) {
    if (!impl || !name) return;
    UniformValue& u = fetchUniform(impl->data, name);
    u.type = UniformType::Vec4;
    u.v4[0] = v.x; u.v4[1] = v.y; u.v4[2] = v.z; u.v4[3] = v.w;
}

void Shader::setUniform(const char* name, const Color& v) {
    if (!impl || !name) return;
    UniformValue& u = fetchUniform(impl->data, name);
    u.type = UniformType::Vec4;
    u.v4[0] = v.r; u.v4[1] = v.g; u.v4[2] = v.b; u.v4[3] = v.a;
}

void Shader::setUniform(const char* name, const Mat4& v) {
    if (!impl || !name) return;
    UniformValue& u = fetchUniform(impl->data, name);
    u.type = UniformType::Mat4;
    std::memcpy(u.m4, v.m, sizeof(v.m));
}

bool Shader::hasUniform(const char* name) const {
    return impl && name && impl->data.uniforms.count(name) > 0;
}

u64 Shader::handle() const {
    return impl ? impl->data.handle : 0;
}

void* Shader::internal() const {
    return impl ? static_cast<void*>(&impl->data) : nullptr;
}

u32 ShaderData::packUniforms(void* dst, u32 maxBytes) const {
    auto* bytes = static_cast<u8*>(dst);
    if (!dst || maxBytes <= SPRITE_PUSH_BYTES) return SPRITE_PUSH_BYTES;

    std::memset(bytes + SPRITE_PUSH_BYTES, 0, maxBytes - SPRITE_PUSH_BYTES);

    u32 offset = SPRITE_PUSH_BYTES;
    for (const std::string& name : uniformOrder) {
        auto it = uniforms.find(name);
        if (it == uniforms.end()) continue;
        const UniformValue& u = it->second;

        u32 size = 4;
        u32 align = 4;
        switch (u.type) {
            case UniformType::Float:
            case UniformType::Int:  size = 4;  align = 4;  break;
            case UniformType::Vec2: size = 8;  align = 8;  break;
            case UniformType::Vec3: size = 12; align = 16; break;
            case UniformType::Vec4: size = 16; align = 16; break;
            case UniformType::Mat4: size = 64; align = 16; break;
        }

        offset = (offset + align - 1) & ~(align - 1);
        if (offset + size > maxBytes) {
            if (!warnedOverflow.exchange(true)) {
                URON_WARN("Shader: uniforms no caben en los " +
                          std::to_string(CUSTOM_PUSH_BYTES) +
                          " bytes de push constants; se ignoran: " + name);
            }
            break;
        }

        switch (u.type) {
            case UniformType::Float: std::memcpy(bytes + offset, &u.f, 4);  break;
            case UniformType::Int:   std::memcpy(bytes + offset, &u.i, 4);  break;
            case UniformType::Vec2:  std::memcpy(bytes + offset, u.v2, 8);  break;
            case UniformType::Vec3:  std::memcpy(bytes + offset, u.v3, 12); break;
            case UniformType::Vec4:  std::memcpy(bytes + offset, u.v4, 16); break;
            case UniformType::Mat4:  std::memcpy(bytes + offset, u.m4, 64); break;
        }
        offset += size;
    }
    return offset;
}

}