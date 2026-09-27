// ============================================================================
//  render/Shader.h
//  ---------------------------------------------------------------------------
//  QUE ES: Shader custom (vertex + fragment) compilados a SPIR-V.
//  CONTIENE: struct ShaderDesc (estado del pipeline), clase Shader con
//            loadFromFile(), setUniform*(), hasUniform().
//  PARA QUE: Que el usuario dibuje con su propio .vert/.frag sin tocar
//            Vulkan. El pipeline GPU se crea en el primer draw.
//  QUIEN LO USA: El usuario (via Sprite2D::setShader).
//
//  PUSH CONSTANTS (128 bytes, ver CLAUDE.md §5):
//     offset  0: vec2 position
//     offset  8: vec2 size
//     offset 16: vec2 screenSize
//     offset 24: uniforms del usuario (alineacion std430)
//  Los uniforms se empaquetan EN EL ORDEN en que se llama a setUniform() la
//  primera vez con cada nombre, que debe coincidir con el orden de
//  declaracion del bloque layout(push_constant) del GLSL. Los no fijados se
//  envian como 0.
//
//  EJEMPLO:
//     Shader s;
//     s.loadFromFile("shaders/wave.vert.spv", "shaders/wave.frag.spv");
//     s.setUniform("time", t);          // mismo orden que en el .vert
//     sprite->setShader(&s);
//  NOTA: loadFromSource() no compila GLSL en runtime: usa glslc (o el build
//        de CMake) para generar los .spv.
// ============================================================================
#pragma once
#include <Uron/Types.h>
#include <Uron/render/Color.h>
#include <Uron/math/Mat4.h>
#include <Uron/math/Vec2.h>
#include <Uron/math/Vec3.h>
#include <Uron/math/Vec4.h>
#include <string>
#include <unordered_map>

namespace Uron {

struct ShaderDesc {
    bool depthTest  = false;
    bool depthWrite = false;
    bool blending   = true;
    bool wireframe  = false;
};

class Shader {
public:
    Shader();
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

    bool isValid() const;

    void setUniform(const char* name, f32 v);
    void setUniform(const char* name, i32 v);
    void setUniform(const char* name, const Vec2& v);
    void setUniform(const char* name, const Vec3& v);
    void setUniform(const char* name, const Vec4& v);
    void setUniform(const char* name, const Color& v);
    void setUniform(const char* name, const Mat4& v);

    bool hasUniform(const char* name) const;

    u64 handle() const;

    void* internal() const;

private:
    struct Impl;
    Impl* impl;
};

}