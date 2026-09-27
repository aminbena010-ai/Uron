// ============================================================================
//  PluginRegistry.h
//  ---------------------------------------------------------------------------
//  QUE ES: Registro global de plugins (singleton).
//  CONTIENE: clase PluginRegistry con get(), registerFactory(), create(),
//            exists(), list().
//  PARA QUE: Que los plugins se auto-registren al cargar el binario.
//            El PluginManager consulta aqui que plugins existen.
//  QUIEN LO USA: Los plugins (se registran con la macro URON_PLUGIN),
//                el PluginManager (crea instancias desde aqui).
//  EJEMPLO:
//     PluginRegistry::get().registerFactory(id, []() { return new Physics(); });
//     IPlugin* p = PluginRegistry::get().create(id);
// ============================================================================
#pragma once
#include <Uron/Plugin/IPlugin.h>
#include <functional>
#include <unordered_map>
#include <vector>

namespace Uron::Plugin {

class PluginRegistry {
public:
    using Factory = std::function<IPlugin*()>;

    static PluginRegistry& get();

    void registerFactory(PluginID id, Factory f);

    IPlugin* create(PluginID id);

    bool exists(PluginID id) const;

    std::vector<PluginID> list() const;

    void clear();

private:
    PluginRegistry() = default;
    std::unordered_map<PluginID, Factory> m_factories;
};

}