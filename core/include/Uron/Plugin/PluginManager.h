// ============================================================================
//  PluginManager.h
//  ---------------------------------------------------------------------------
//  QUE ES: El gestor central de plugins del motor.
//  CONTIENE: clase PluginManager con load<T>(), unload(), updateAll(),
//            get<T>(), context(), isLoaded(), count().
//  PARA QUE: Cargar, descargar, actualizar y organizar todos los plugins.
//            Es lo que usa Engine internamente.
//  QUIEN LO USA: El motor (Engine lo tiene como miembro).
//  EJEMPLO:
//     m_pluginManager.load<Physics::Plugin>();
//     m_pluginManager.updateAll(dt);
// ============================================================================
#pragma once
#include <Uron/Plugin/IPlugin.h>
#include <Uron/Plugin/PluginContext.h>
#include <memory>
#include <unordered_map>

namespace Uron {
class Engine;
}

namespace Uron::Plugin {

class PluginManager {
public:
    explicit PluginManager(Engine& engine);

    template<typename T>
    bool load();

    void unload(PluginID id);
    void unloadAll();

    void updateAll(float dt);

    template<typename T>
    T* get();

    IPlugin* getByID(PluginID id);
    IPlugin* getByName(const char* name);

    PluginContext& context(PluginID id);

    bool   isLoaded(PluginID id) const;
    size_t count() const;

private:
    Engine& m_engine;
    std::unordered_map<PluginID, std::unique_ptr<IPlugin>> m_plugins;
    std::unordered_map<PluginID, PluginContext>            m_contexts;
};

template<typename T>
bool PluginManager::load() {
    auto plugin = std::make_unique<T>();
    PluginID id = plugin->id();

    if (m_plugins.count(id)) return false;

    m_contexts.emplace(id, PluginContext(m_engine, id));
    if (!plugin->onLoad(m_contexts.at(id))) {
        m_contexts.erase(id);
        return false;
    }

    plugin->markLoaded(true);
    m_plugins[id] = std::move(plugin);
    return true;
}

template<typename T>
T* PluginManager::get() {
    auto it = m_plugins.find(T::ID);
    if (it == m_plugins.end()) return nullptr;
    return static_cast<T*>(it->second.get());
}

}