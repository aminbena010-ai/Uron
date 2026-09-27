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
#include <type_traits>
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

    // Adopta un plugin ya construido (lo usa Engine::importPlugin<T>):
    // crea contexto, llama onLoad, registra por plugin->id(). Devuelve
    // false si id() ya estaba cargado o onLoad falla.
    bool adopt(std::unique_ptr<IPlugin> plugin);

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
    static_assert(std::is_base_of_v<IPlugin, T>,
                  "T debe heredar de Uron::Plugin::IPlugin");
    static_assert(detail::HasPluginID<T>::value,
                  "T debe definir `static constexpr PluginID ID` "
                  "(estandar de plugins, CLAUDE.md §6)");
    if (m_plugins.count(T::ID)) return false;
    return adopt(std::make_unique<T>());
}

template<typename T>
T* PluginManager::get() {
    static_assert(std::is_base_of_v<IPlugin, T>,
                  "T debe heredar de Uron::Plugin::IPlugin");
    static_assert(detail::HasPluginID<T>::value,
                  "T debe definir `static constexpr PluginID ID` "
                  "(estandar de plugins, CLAUDE.md §6)");
    auto it = m_plugins.find(T::ID);
    if (it == m_plugins.end()) return nullptr;
    return static_cast<T*>(it->second.get());
}

}