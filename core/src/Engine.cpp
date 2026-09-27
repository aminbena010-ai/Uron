#include <Uron/Engine.h>
#include <Uron/Uron.h>
#include <Uron/Logger.h>
#include <Uron/Plugin/IPlugin.h>
#include <Uron/Plugin/PluginContext.h>
#include <Uron/Plugin/PluginManager.h>
#include <Uron/Plugin/EventBus.h>
#include <Uron/Plugin/ServiceRegistry.h>
#include <Uron/scene/Scene2D.h>

#include <vulkan/VulkanRenderer.h>

#include <chrono>
#include <unordered_map>
#include <typeinfo>
#include <memory>
#include <string>

namespace Uron {

struct Engine::Impl {
    bool initialized = false;
    bool running     = false;

    Window* window = nullptr;
    Input   input;

    // UN solo sistema de plugins (BUG-010/011): todo pasa por aqui.
    std::unique_ptr<Plugin::PluginManager> pluginManager;

    // Bus/registro compartidos por todos los plugins (BUG-026).
    Plugin::EventBus        eventBus;
    Plugin::ServiceRegistry serviceRegistry;

    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point startTime;
    Clock::time_point lastFrame;
    f32 deltaTime = 0.f;
    f64 totalTime = 0.f;

    Vulkan::VulkanRenderer* renderer = nullptr;
    Scene2D* scene = nullptr;
};

Engine::Engine() : impl(new Impl) {}

Engine::~Engine() {
    shutdown();
    delete impl;
}

bool Engine::init() {
    if (impl->initialized) {
        URON_WARN("Engine::init llamado dos veces. Ignorando.");
        return true;
    }

    Logger::info("===========================================");
    Logger::info("  Uron Engine " URON_VERSION_STRING);
    Logger::info("  Ligero, flexible, letal.");
    Logger::info("===========================================");

    impl->pluginManager = std::make_unique<Plugin::PluginManager>(*this);
    impl->startTime     = Impl::Clock::now();
    impl->lastFrame     = impl->startTime;
    impl->initialized   = true;
    impl->running       = true;

    URON_INFO("Engine inicializado (sin ventana, sin Vulkan)");
    return true;
}

void Engine::shutdown() {
    if (!impl || !impl->initialized) return;

    URON_INFO("Apagando Uron Engine...");

    if (impl->scene) {
        impl->scene->exit();
        impl->scene = nullptr;
    }

    // Un solo lugar para descargar: unloadAll() llama onUnload() UNA vez
    // por plugin y destruye contextos + instancias (BUG-010/011).
    if (impl->pluginManager) {
        impl->pluginManager->unloadAll();
        impl->pluginManager.reset();
    }

    impl->eventBus.clear();

    if (impl->renderer) {
        impl->renderer->shutdown();
        delete impl->renderer;
        impl->renderer = nullptr;
    }

    impl->window      = nullptr;
    impl->initialized = false;
    impl->running     = false;

    URON_INFO("Uron Engine apagado.");
}

bool Engine::isInitialized() const {
    return impl->initialized;
}

bool Engine::attachWindow(Window* window) {
    if (!impl->initialized) {
        URON_ERROR("Engine no inicializado. Llama a init() antes.");
        return false;
    }
    if (!window || !window->isValid()) {
        URON_ERROR("Ventana invalida.");
        return false;
    }

    impl->window = window;

    URON_INFO("Ventana adjuntada al engine");
    URON_INFO("  - Inicializando Vulkan...");

    impl->renderer = new Vulkan::VulkanRenderer();
    if (!impl->renderer->init(window)) {
        URON_ERROR("Fallo al inicializar VulkanRenderer");
        delete impl->renderer;
        impl->renderer = nullptr;
        impl->window = nullptr;
        return false;
    }

    URON_INFO("Vulkan listo");

    impl->input.reset();
    WindowCallbacks inputCbs;
    Input* in = &impl->input;
    inputCbs.onKey         = [in](i32 key, i32 action) { in->handleKey(key, action); };
    inputCbs.onMouseButton = [in](i32 button, i32 action) { in->handleMouseButton(button, action); };
    inputCbs.onMouseMove   = [in](f32 x, f32 y) { in->handleMouseMove(x, y); };
    inputCbs.onScroll      = [in](f32 dx, f32 dy) { in->handleScroll(dx, dy); };
    window->setEngineCallbacks(inputCbs);

    if (impl->scene) {
        impl->scene->enter();
    }

    return true;
}

void Engine::detachWindow() {
    if (!impl->window) return;

    if (impl->scene) {
        impl->scene->exit();
    }

    impl->window->setEngineCallbacks({});
    impl->input.reset();

    if (impl->renderer) {
        impl->renderer->shutdown();
        delete impl->renderer;
        impl->renderer = nullptr;
    }

    impl->window = nullptr;
    URON_INFO("Ventana desadjuntada del engine");
}

Window* Engine::window() const {
    return impl->window;
}

bool Engine::hasWindow() const {
    return impl->window != nullptr;
}

Input& Engine::input() const {
    return impl->input;
}

void Engine::beginFrame() {
    if (!impl->running) return;

    // Promueve los bordes (pressed/released) y deltas acumulados en el
    // pollEvents() anterior: todo lo que sigue ve un estado estable.
    impl->input.beginFrame();

    auto now = Impl::Clock::now();
    impl->deltaTime = std::chrono::duration<f32>(now - impl->lastFrame).count();
    impl->totalTime = std::chrono::duration<f64>(now - impl->startTime).count();
    impl->lastFrame = now;

    if (impl->scene) {
        impl->scene->update(impl->deltaTime);
    }

    if (impl->pluginManager) {
        impl->pluginManager->updateAll(impl->deltaTime);
    }

    if (impl->renderer) {
        impl->renderer->beginFrame();
    }
}

void Engine::clear(const Color& c) {
    if (impl->renderer) {
        impl->renderer->clear(c);
    }
}

void Engine::endFrame() {
    if (impl->scene && impl->renderer) {
        impl->scene->render(*impl->renderer);
    }

    if (impl->renderer) {
        impl->renderer->endFrame();
    }
}

f32 Engine::deltaTime() const { return impl->deltaTime; }
f64 Engine::totalTime() const { return impl->totalTime; }

void Engine::setScene(Scene2D* scene) {
    if (impl->scene == scene) return;

    if (impl->scene) {
        impl->scene->exit();
    }

    impl->scene = scene;

    if (impl->scene) {
        impl->scene->enter();
    }
}

Scene2D* Engine::scene() const {
    return impl->scene;
}

bool Engine::internalImportPlugin_(Plugin::PluginID id, void* (*factory)()) {
    if (!impl->pluginManager || !factory) return false;

    if (impl->pluginManager->isLoaded(id)) {
        URON_WARN("Plugin ya importado (PluginID " + std::to_string(id) + ")");
        return true;
    }

    void* raw = factory();
    if (!raw) {
        URON_ERROR("No se pudo crear plugin (PluginID " +
                   std::to_string(id) + ")");
        return false;
    }

    std::unique_ptr<Plugin::IPlugin> plugin(
        static_cast<Plugin::IPlugin*>(raw));

    if (plugin->id() != id) {
        URON_WARN(std::string("Plugin ") + plugin->name() +
                  ": id() no coincide con T::ID (CLAUDE.md §6)");
    }

    if (!impl->pluginManager->adopt(std::move(plugin))) {
        URON_ERROR("onLoad fallo para plugin (PluginID " +
                   std::to_string(id) + ")");
        return false;
    }

    URON_INFO("Plugin importado: " +
              std::string(impl->pluginManager->getByID(id)->name()));
    return true;
}

void* Engine::internalGetPlugin_(Plugin::PluginID id) {
    if (!impl->pluginManager) return nullptr;
    return impl->pluginManager->getByID(id);
}

bool Engine::isPluginLoaded(const char* name) const {
    if (!impl->pluginManager || !name) return false;
    // Rapido: ID = makePluginID(name) (convencion CLAUDE.md §6).
    if (impl->pluginManager->isLoaded(Plugin::makePluginID(name))) return true;
    // Fallback: plugins cuyo ID no es hash(nombre).
    return impl->pluginManager->getByName(name) != nullptr;
}

bool Engine::isPluginLoaded(Plugin::PluginID id) const {
    return impl->pluginManager && impl->pluginManager->isLoaded(id);
}

Plugin::EventBus& Engine::eventBus() {
    return impl->eventBus;
}

Plugin::ServiceRegistry& Engine::serviceRegistry() {
    return impl->serviceRegistry;
}

void* Engine::internal() {
    return impl;
}

}