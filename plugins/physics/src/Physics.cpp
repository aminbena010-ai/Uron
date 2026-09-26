#include <Uron/Plugin/Physics.h>
#include <Uron/Plugin/PluginContext.h>
#include <Uron/Logger.h>

namespace Uron::Plugin::Physics {

bool Plugin::onLoad(PluginContext& ctx) {
    m_ctx = &ctx;

    ctx.services().provide<Plugin>("physics", this);
    ctx.api().logInfo("Physics plugin loaded");

    return true;
}

void Plugin::onUnload(PluginContext& ctx) {
    ctx.api().logInfo("Physics plugin unloaded");
    m_bodies.clear();
    m_ctx = nullptr;
}

void Plugin::onUpdate(PluginContext& ctx, float dt) {
    (void)ctx;
    step(dt);
}

void Plugin::setGravity(const Vec3& g) {
    m_gravity = g;
}

Vec3 Plugin::gravity() const {
    return m_gravity;
}

BodyID Plugin::createBody(const BodyDesc& desc) {
    Body b;
    b.id          = m_nextId++;
    b.position    = desc.position;
    b.velocity    = desc.velocity;
    b.size        = desc.size;
    b.mass        = desc.mass;
    b.restitution = desc.restitution;
    b.isStatic    = desc.isStatic;

    m_bodies.push_back(b);
    return b.id;
}

void Plugin::destroyBody(BodyID id) {
    for (auto it = m_bodies.begin(); it != m_bodies.end(); ++it) {
        if (it->id == id) {
            m_bodies.erase(it);
            return;
        }
    }
}

Body* Plugin::getBody(BodyID id) {
    for (auto& b : m_bodies) {
        if (b.id == id) return &b;
    }
    return nullptr;
}

void Plugin::step(float dt) {
    for (auto& b : m_bodies) {
        if (b.isStatic) continue;
        b.velocity += m_gravity * dt;
        b.position += b.velocity * dt;
    }

    const float groundY = 0.f;
    for (auto& b : m_bodies) {
        if (b.isStatic) continue;
        float halfSize = b.size.y * 0.5f;
        if (b.position.y - halfSize < groundY) {
            b.position.y = groundY + halfSize;
            b.velocity.y = -b.velocity.y * b.restitution;
            if (std::abs(b.velocity.y) < 0.01f) b.velocity.y = 0.f;
        }
    }
}

void Plugin::clear() {
    m_bodies.clear();
    m_nextId = 1;
}

}

URON_PLUGIN(Physics, Plugin)