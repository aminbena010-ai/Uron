#pragma once
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>
#include <any>
#include <algorithm>

namespace Uron::Plugin {

struct Event {
    const char* type = "";
    std::any    data;
};

class EventBus {
public:
    using Handler = std::function<void(const Event&)>;

    uint32_t subscribe(const char* type, Handler h) {
        uint32_t id = m_nextId++;
        m_handlers[type].push_back({id, std::move(h)});
        return id;
    }

    void unsubscribe(const char* type, uint32_t id) {
        auto it = m_handlers.find(type);
        if (it == m_handlers.end()) return;
        auto& vec = it->second;
        vec.erase(std::remove_if(vec.begin(), vec.end(),
            [id](const auto& p) { return p.first == id; }), vec.end());
    }

    void publish(const Event& e) {
        auto it = m_handlers.find(e.type);
        if (it == m_handlers.end()) return;
        for (auto& [id, h] : it->second) {
            if (h) h(e);
        }
    }

    void clear() {
        m_handlers.clear();
    }

private:
    std::unordered_map<const char*, std::vector<std::pair<uint32_t, Handler>>> m_handlers;
    uint32_t m_nextId = 0;
};

}