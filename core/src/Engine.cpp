#include <Uron/Engine.h>
#include <Uron/Logger.h>
#include <Uron/Plugin/IPlugin.h>
#include <Uron/Plugin/PluginContext.h>
#include <Uron/Plugin/PluginManager.h>
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

    std::unique_ptr<Plugin::PluginManager> pluginManager;

    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point startTime;
    Clock::time_point lastFrame;
    f32 deltaTime = 0.f;
    f64 totalTime = 0.f;

    std::unordered_map<std::string, void*> plugins;
    std::unordered_map<std::string, Plugin::PluginContext*> contexts;

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
    Logger::info("  Uron Engine 0.3.0");
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

    for (auto& [typeName, rawPlugin] : impl->plugins) {
        auto* plugin = static_cast<Plugin::IPlugin*>(rawPlugin);
        auto it = impl->contexts.find(typeName);
        if (plugin && it != impl->contexts.end() && it->second) {
            plugin->onUnload(*it->second);
        }
    }

    for (auto& [typeName, ctx] : impl->contexts) {
        delete ctx;
    }
    impl->contexts.clear();

    for (auto& [typeName, rawPlugin] : impl->plugins) {
        auto* plugin = static_cast<Plugin::IPlugin*>(rawPlugin);
        delete plugin;
    }
    impl->plugins.clear();

    if (impl->pluginManager) {
        impl->pluginManager->unloadAll();
        impl->pluginManager.reset();
    }

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
    return true;
}

void Engine::detachWindow() {
    if (!impl->window) return;

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

void Engine::beginFrame() {
    if (!impl->running) return;

    auto now = Impl::Clock::now();
    impl->deltaTime = std::chrono::duration<f32>(now - impl->lastFrame).count();
    impl->totalTime = std::chrono::duration<f64>(now - impl->startTime).count();
    impl->lastFrame = now;

    if (impl->scene) {
        impl->scene->update(impl->deltaTime);
    }

    for (auto& [typeName, rawPlugin] : impl->plugins) {
        auto* plugin = static_cast<Plugin::IPlugin*>(rawPlugin);
        auto it = impl->contexts.find(typeName);
        if (plugin && it != impl->contexts.end() && it->second) {
            plugin->onUpdate(*it->second, impl->deltaTime);
        }
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

bool Engine::internalImportPlugin_(const char* typeName, void* (*factory)()) {
    if (!impl->pluginManager) return false;

    auto it = impl->plugins.find(typeName);
    if (it != impl->plugins.end()) {
        URON_WARN(std::string("Plugin ya importado: ") + typeName);
        return true;
    }

    void* raw = factory();
    if (!raw) {
        URON_ERROR(std::string("No se pudo crear plugin: ") + typeName);
        return false;
    }

    auto* plugin = static_cast<Plugin::IPlugin*>(raw);

    auto* ctx = new Plugin::PluginContext(*this, plugin->id());

    if (!plugin->onLoad(*ctx)) {
        URON_ERROR(std::string("onLoad fallo para plugin: ") + typeName);
        delete ctx;
        delete plugin;
        return false;
    }

    plugin->markLoaded(true);

    impl->plugins[typeName]  = plugin;
    impl->contexts[typeName] = ctx;

    URON_INFO(std::string("Plugin importado: ") + plugin->name());
    return true;
}

void* Engine::internalGetPlugin_(const char* typeName) {
    auto it = impl->plugins.find(typeName);
    return it != impl->plugins.end() ? it->second : nullptr;
}

bool Engine::isPluginLoaded(const char* name) const {
    for (auto& [_, p] : impl->plugins) {
        auto* plugin = static_cast<Plugin::IPlugin*>(p);
        if (plugin && std::string(plugin->name()) == name) return true;
    }
    return false;
}

void* Engine::internal() {
    return impl;
}

}