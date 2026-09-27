// ============================================================================
//  ServiceRegistry.h
//  ---------------------------------------------------------------------------
//  QUE ES: Registro de servicios entre plugins.
//  CONTIENE: clase ServiceRegistry con provide<T>(), consume<T>(), has().
//  PARA QUE: Que un plugin ofrezca un servicio y otro lo consuma
//            SIN depender directamente del otro. Ambos hablan con el motor.
//  QUIEN LO USA: Los plugins (ofrecen y consumen servicios).
//  EJEMPLO:
//     ctx.services().provide<PhysicsService>("physics", &m_service);
//     auto* p = ctx.services().consume<PhysicsService>("physics");
// ============================================================================
#pragma once
#include <string>
#include <unordered_map>
#include <typeinfo>

namespace Uron::Plugin {

class ServiceRegistry {
public:
    template<typename T>
    void provide(const std::string& name, T* service) {
        m_services[name] = static_cast<void*>(service);
        m_types[name]    = &typeid(T);
    }

    template<typename T>
    T* consume(const std::string& name) {
        auto it = m_services.find(name);
        if (it == m_services.end()) return nullptr;
        if (m_types[name] != &typeid(T)) return nullptr;
        return static_cast<T*>(it->second);
    }

    template<typename T>
    const T* consume(const std::string& name) const {
        auto it = m_services.find(name);
        if (it == m_services.end()) return nullptr;
        if (m_types.at(name) != &typeid(T)) return nullptr;
        return static_cast<const T*>(it->second);
    }

    bool has(const std::string& name) const {
        return m_services.count(name) > 0;
    }

    void remove(const std::string& name) {
        m_services.erase(name);
        m_types.erase(name);
    }

    void clear() {
        m_services.clear();
        m_types.clear();
    }

private:
    std::unordered_map<std::string, void*>                 m_services;
    std::unordered_map<std::string, const std::type_info*> m_types;
};

}