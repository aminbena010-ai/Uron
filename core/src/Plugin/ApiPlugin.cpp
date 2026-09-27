#include <Uron/Plugin/ApiPlugin.h>
#include <Uron/Plugin/EventBus.h>
#include <Uron/Engine.h>
#include <Uron/Logger.h>

namespace Uron::Plugin {

void ApiPlugin::onUpdate(std::function<void(float)> cb) {
    m_updateCb = std::move(cb);
}

void ApiPlugin::onRender(std::function<void()> cb) {
    m_renderCb = std::move(cb);
}

void ApiPlugin::onEvent(std::function<void(const Event&)> cb) {
    m_eventCb = std::move(cb);
}

void* ApiPlugin::renderer() {
    return nullptr;
}

void* ApiPlugin::scene2D() {
    return nullptr;
}

void* ApiPlugin::scene3D() {
    return nullptr;
}

void* ApiPlugin::loadTexture(const char* path) {
    (void)path;
    return nullptr;
}

void* ApiPlugin::loadShader(const char* path) {
    (void)path;
    return nullptr;
}

void ApiPlugin::logInfo(const char* msg) {
    Logger::info(msg);
}

void ApiPlugin::logWarn(const char* msg) {
    Logger::warn(msg);
}

void ApiPlugin::logError(const char* msg) {
    Logger::error(msg);
}

float ApiPlugin::deltaTime() const {
    return m_engine ? m_engine->deltaTime() : 0.f;
}

double ApiPlugin::totalTime() const {
    return m_engine ? m_engine->totalTime() : 0.0;
}

ServiceRegistry& ApiPlugin::services() {
    return m_services;
}

}