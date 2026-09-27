// ============================================================================
//  render/Shader.h
//  ---------------------------------------------------------------------------
//  QUE ES: Programa de shader (vertex + fragment).
//  CONTIENE: struct ShaderDesc, clase Shader con loadFromFile(),
//            loadFromSource(), setUniform*(), bind().
//  PARA QUE: Que el usuario cargue shaders (SPIR-V compilados) y los use
//            para dibujar, sin tocar Vulkan.
//  QUIEN LO USA: El usuario (shaders custom), el motor (shader por defecto).
//  NOTA: El motor compila GLSL -> SPIR-V en build time con glslangValidator.
//        El usuario puede pasar .vert/.frag o .spv directamente.
//  EJEMPLO:
//     Shader s;
//     s.loadFromFile("sprite.vert", "sprite.frag");
// ============================================================================
#pragma once
#include <Uron/Types.h>
#include <Uron/render/Color.h>
#include <Uron/math/Mat4.h>
#include <Uron/math/Vec2.h>
#include <Uron/math/Vec3.h>
#include <Uron/math/Vec4.h>
#include <string>

namespace Uron {

struct ShaderDesc {
    bool depthTest  = true;
    bool depthWrite = true;
    bool blending   = false;
    bool wireframe  = false;
};

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) noexcept;
    Shader& operator=(Shader&&) noexcept;

    bool loadFromFile(const std::string& vertPath,
                      const std::string& fragPath,
                      const ShaderDesc& desc = {});

    bool loadFromSource(const std::string& vertSrc,
                        const std::string& fragSrc,
                        const ShaderDesc& desc = {});

    void destroy();
    void bind();
    void unbind();

    bool isValid() const { return m_handle != 0; }

    void setUniform(const char* name, f32 v);
    void setUniform(const char* name, i32 v);
    void setUniform(const char* name, const Vec2& v);
    void setUniform(const char* name, const Vec3& v);
    void setUniform(const char* name, const Vec4& v);
    void setUniform(const char* name, const Color& v);
    void setUniform(const char* name, const Mat4& v);

    u64 handle() const { return m_handle; }

private:
    u64 m_handle = 0;

    friend class VulkanRenderer;
};

}