#pragma once
#include <Uron/Plugin/ServiceRegistry.h>
#include <functional>

namespace Uron {
class Engine;
}

namespace Uron::Plugin {

struct Event;

class ApiPlugin {
public:
    explicit ApiPlugin(Engine& engine) : m_engine(&engine) {}

    void onUpdate(std::function<void(float)> cb);
    void onRender(std::function<void()> cb);
    void onEvent(std::function<void(const Event&)> cb);

    void* renderer();
    void* scene2D();
    void* scene3D();

    void* loadTexture(const char* path);
    void* loadShader(const char* path);

    void logInfo(const char* msg);
    void logWarn(const char* msg);
    void logError(const char* msg);

    float  deltaTime() const;
    double totalTime() const;

    ServiceRegistry& services();

private:
    Engine* m_engine = nullptr;

    std::function<void(float)>        m_updateCb;
    std::function<void()>             m_renderCb;
    std::function<void(const Event&)> m_eventCb;

    ServiceRegistry m_services;
};

}