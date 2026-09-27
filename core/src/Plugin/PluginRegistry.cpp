#include <Uron/Plugin/PluginRegistry.h>

namespace Uron::Plugin {

PluginRegistry& PluginRegistry::get() {
    static PluginRegistry instance;
    return instance;
}

void PluginRegistry::registerFactory(PluginID id, Factory f) {
    m_factories[id] = std::move(f);
}

IPlugin* PluginRegistry::create(PluginID id) {
    auto it = m_factories.find(id);
    if (it == m_factories.end()) return nullptr;
    return it->second();
}

bool PluginRegistry::exists(PluginID id) const {
    return m_factories.count(id) > 0;
}

std::vector<PluginID> PluginRegistry::list() const {
    std::vector<PluginID> ids;
    ids.reserve(m_factories.size());
    for (auto& [id, _] : m_factories) ids.push_back(id);
    return ids;
}

void PluginRegistry::clear() {
    m_factories.clear();
}

}