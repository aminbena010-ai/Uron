#pragma once
#include <Uron/Uron.h>
#include <Uron/Plugin/IPlugin.h>
#include <Uron/Plugin/PluginID.h>
#include <Uron/Plugin/Macros.h>
#include <Uron/math/Vec3.h>

#include <vector>

namespace Uron::Plugin::Physics {

using BodyID = u32;
constexpr BodyID InvalidBody = 0;

struct BodyDesc {
    Vec3  position  = {0.f, 0.f, 0.f};
    Vec3  velocity  = {0.f, 0.f, 0.f};
    Vec3  size      = {1.f, 1.f, 1.f};
    float mass      = 1.f;
    float restitution = 0.f;
    bool  isStatic  = false;
};

struct Body {
    BodyID id         = InvalidBody;
    Vec3   position   = {0.f, 0.f, 0.f};
    Vec3   velocity   = {0.f, 0.f, 0.f};
    Vec3   size       = {1.f, 1.f, 1.f};
    float  mass       = 1.f;
    float  restitution = 0.f;
    bool   isStatic   = false;
};

class Plugin final : public IPlugin {
public:
    static constexpr PluginID  ID      = makePluginID("Physics");
    static constexpr const char* NAME  = "Physics";
    static constexpr const char* VERSION = "1.0.0";
    static constexpr const char* AUTHOR  = "Uron Official";

    PluginID    id()      const override { return ID; }
    const char* name()    const override { return NAME; }
    const char* version() const override { return VERSION; }
    const char* author()  const override { return AUTHOR; }

    bool onLoad(PluginContext& ctx) override;
    void onUnload(PluginContext& ctx) override;
    void onUpdate(PluginContext& ctx, float dt) override;

    void setGravity(const Vec3& g);
    Vec3 gravity() const;

    BodyID createBody(const BodyDesc& desc);
    void   destroyBody(BodyID id);
    Body*  getBody(BodyID id);
    const std::vector<Body>& bodies() const { return m_bodies; }

    void step(float dt);
    void clear();

private:
    PluginContext* m_ctx = nullptr;
    Vec3           m_gravity{0.f, -9.8f, 0.f};
    std::vector<Body> m_bodies;
    BodyID         m_nextId = 1;
};

}