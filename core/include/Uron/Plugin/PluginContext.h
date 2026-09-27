#pragma once
#include <Uron/Plugin/PluginID.h>
#include <Uron/Plugin/EventBus.h>
#include <Uron/Plugin/ServiceRegistry.h>
#include <Uron/Plugin/ApiPlugin.h>

namespace Uron {
class Engine;
}

namespace Uron::Plugin {

class PluginContext {
public:
    PluginContext(Engine& engine, PluginID id);

    Engine&          engine();
    EventBus&        events();
    ServiceRegistry& services();
    ApiPlugin&       api();

    PluginID id() const { return m_id; }

private:
    Engine&         m_engine;
    PluginID        m_id;
    EventBus        m_eventBus;
    ServiceRegistry m_services;
    ApiPlugin       m_api;
};

}