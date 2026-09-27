#include <Uron/Plugin/PluginContext.h>
#include <Uron/Engine.h>

namespace Uron::Plugin {

PluginContext::PluginContext(Engine& engine, PluginID id)
    : m_engine(engine)
    , m_id(id)
    , m_api(engine)
{
}

Engine& PluginContext::engine() {
    return m_engine;
}

EventBus& PluginContext::events() {
    return m_eventBus;
}

ServiceRegistry& PluginContext::services() {
    return m_services;
}

ApiPlugin& PluginContext::api() {
    return m_api;
}

}