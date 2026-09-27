#pragma once
#include <Uron/Types.h>
#include <Uron/Window.h>
#include <Uron/Input.h>
#include <Uron/Logger.h>
#include <Uron/render/Color.h>
#include <Uron/Plugin/PluginID.h>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>

namespace Uron {

class Scene2D;

namespace Plugin {
class IPlugin;
class EventBus;
class ServiceRegistry;
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

    // El puntero es NO PROPIEDAD: window debe vivir hasta detachWindow()
    // (o hasta ~Engine si no se desvincula). No llamar con una Window
    // apilada que muera antes que el motor.
    bool attachWindow(Window* window);
    void detachWindow();

    Window* window() const;
    bool    hasWindow() const;

    Input& input() const;

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
    bool isPluginLoaded(Plugin::PluginID id) const;

    // Bus/registro compartidos por TODOS los plugins (BUG-026): la
    // comunicacion entre plugins pasa por aqui, no por contextos privados.
    Plugin::EventBus&        eventBus();
    Plugin::ServiceRegistry& serviceRegistry();

    void* internal();

private:
    // Clave = PluginID (T::ID), nunca typeid().name() (BUG-033).
    bool  internalImportPlugin_(Plugin::PluginID id, void* (*factory)());
    void* internalGetPlugin_(Plugin::PluginID id);

    struct Impl;
    Impl* impl;
};

template<typename TPlugin>
bool Engine::importPlugin() {
    static_assert(std::is_base_of_v<Plugin::IPlugin, TPlugin>,
                  "TPlugin debe heredar de Uron::Plugin::IPlugin");
    static_assert(Plugin::detail::HasPluginID<TPlugin>::value,
                  "TPlugin debe definir `static constexpr PluginID ID` "
                  "(estandar de plugins, CLAUDE.md §6)");
    return internalImportPlugin_(TPlugin::ID,
                                 []() -> void* { return new TPlugin(); });
}

template<typename TPlugin>
TPlugin& Engine::plugin() {
    TPlugin* p = tryPlugin<TPlugin>();
    if (!p) {
        URON_FATAL(std::string("Engine::plugin<T>() llamado sin importar: ") +
                   typeid(TPlugin).name());
    }
    return *p;
}

template<typename TPlugin>
TPlugin* Engine::tryPlugin() {
    static_assert(Plugin::detail::HasPluginID<TPlugin>::value,
                  "TPlugin debe definir `static constexpr PluginID ID`");
    return static_cast<TPlugin*>(internalGetPlugin_(TPlugin::ID));
}

}