// ============================================================================
//  PluginConfig.h
//  ---------------------------------------------------------------------------
//  QUE ES: Configuracion por plugin (key-value).
//  CONTIENE: clase PluginConfig con set(), get(), getFloat(), getBool(),
//            has(), remove(), clear().
//  PARA QUE: Que cada plugin tenga su propia configuracion
//            (por ejemplo leida de un JSON o pasada por el usuario).
//  QUIEN LO USA: Los plugins (leen su config), el usuario (la define).
//  EJEMPLO:
//     PluginConfig cfg;
//     cfg.set("gravity", "-9.8");
//     float g = cfg.getFloat("gravity");
// ============================================================================
#pragma once
#include <string>
#include <unordered_map>
#include <cstdlib>

namespace Uron::Plugin {

class PluginConfig {
public:
    void set(const std::string& key, const std::string& value) {
        m_values[key] = value;
    }

    std::string get(const std::string& key, const std::string& def = "") const {
        auto it = m_values.find(key);
        return it != m_values.end() ? it->second : def;
    }

    float getFloat(const std::string& key, float def = 0.f) const {
        auto it = m_values.find(key);
        return it != m_values.end() ? std::stof(it->second) : def;
    }

    int getInt(const std::string& key, int def = 0) const {
        auto it = m_values.find(key);
        return it != m_values.end() ? std::stoi(it->second) : def;
    }

    bool getBool(const std::string& key, bool def = false) const {
        auto it = m_values.find(key);
        return it != m_values.end() ? (it->second == "true" || it->second == "1") : def;
    }

    bool has(const std::string& key) const {
        return m_values.count(key) > 0;
    }

    void remove(const std::string& key) {
        m_values.erase(key);
    }

    void clear() {
        m_values.clear();
    }

private:
    std::unordered_map<std::string, std::string> m_values;
};

}