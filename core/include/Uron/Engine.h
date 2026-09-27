#pragma once
#include <Uron/Types.h>
#include <Uron/Window.h>
#include <Uron/render/Color.h>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>

namespace Uron {

class Scene2D;

namespace Plugin {
class IPlugin;
}

class Engine {
public:
    Engine();
    ~Engine();

    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;

    bool init();
    void shutdown();

    bool isInitialized() const;

    bool attachWindow(Window* window);
    void detachWindow();

    Window* window() const;
    bool    hasWindow() const;

    void beginFrame();
    void clear(const Color& c);
    void endFrame();

    f32 deltaTime() const;
    f64 totalTime() const;

    void setScene(Scene2D* scene);
    Scene2D* scene() const;

    template<typename TPlugin>
    bool importPlugin();

    template<typename TPlugin>
    TPlugin& plugin();

    template<typename TPlugin>
    TPlugin* tryPlugin();

    bool isPluginLoaded(const char* name) const;

    void* internal();

private:
    bool  internalImportPlugin_(const char* typeName, void* (*factory)());
    void* internalGetPlugin_(const char* typeName);

    struct Impl;
    Impl* impl;
};

template<typename TPlugin>
bool Engine::importPlugin() {
    static_assert(std::is_base_of_v<Plugin::IPlugin, TPlugin>,
                  "TPlugin debe heredar de Uron::Plugin::IPlugin");
    return internalImportPlugin_(typeid(TPlugin).name(),
                                 []() -> void* { return new TPlugin(); });
}

template<typename TPlugin>
TPlugin& Engine::plugin() {
    TPlugin* p = tryPlugin<TPlugin>();
    return *p;
}

template<typename TPlugin>
TPlugin* Engine::tryPlugin() {
    return static_cast<TPlugin*>(internalGetPlugin_(typeid(TPlugin).name()));
}

}