#include <Uron/Plugin/PluginManager.h>
#include <Uron/Engine.h>
#include <Uron/Logger.h>

namespace Uron::Plugin {

PluginManager::PluginManager(Engine& engine)
    : m_engine(engine)
{
}

void PluginManager::unload(PluginID id) {
    auto it = m_plugins.find(id);
    if (it == m_plugins.end()) return;

    auto ctxIt = m_contexts.find(id);
    if (ctxIt != m_contexts.end()) {
        it->second->onUnload(ctxIt->second);
    }

    it->second->markLoaded(false);
    m_plugins.erase(it);
    m_contexts.erase(id);
}

void PluginManager::unloadAll() {
    std::vector<PluginID> ids;
    ids.reserve(m_plugins.size());
    for (auto& [id, _] : m_plugins) ids.push_back(id);

    for (PluginID id : ids) unload(id);
}

void PluginManager::updateAll(float dt) {
    for (auto& [id, plugin] : m_plugins) {
        auto ctxIt = m_contexts.find(id);
        if (ctxIt != m_contexts.end()) {
            plugin->onUpdate(ctxIt->second, dt);
        }
    }
}

IPlugin* PluginManager::getByID(PluginID id) {
    auto it = m_plugins.find(id);
    return it != m_plugins.end() ? it->second.get() : nullptr;
}

IPlugin* PluginManager::getByName(const char* name) {
    for (auto& [id, plugin] : m_plugins) {
        if (plugin && std::string(plugin->name()) == name) return plugin.get();
    }
    return nullptr;
}

PluginContext& PluginManager::context(PluginID id) {
    return m_contexts.at(id);
}

bool PluginManager::isLoaded(PluginID id) const {
    return m_plugins.count(id) > 0;
}

size_t PluginManager::count() const {
    return m_plugins.size();
}

}