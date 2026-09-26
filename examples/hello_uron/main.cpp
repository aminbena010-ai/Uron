#include <Uron/Uron.h>
#include <cstdio>

int main() {
    Uron::Engine engine;
    if (!engine.init()) {
        URON_ERROR("No se pudo inicializar el engine");
        return -1;
    }

    Uron::Window window;
    Uron::WindowConfig cfg;
    cfg.title  = "Hello Uron";
    cfg.width  = 1280;
    cfg.height = 720;

    if (!window.create(cfg)) {
        URON_ERROR("No se pudo crear la ventana");
        engine.shutdown();
        return -1;
    }

    if (!engine.attachWindow(&window)) {
        URON_ERROR("No se pudo adjuntar la ventana");
        window.destroy();
        engine.shutdown();
        return -1;
    }

    while (!window.shouldClose()) {
        window.pollEvents();
        engine.beginFrame();
        engine.clear(Uron::Color::Black());
        engine.endFrame();
    }

    window.destroy();
    engine.shutdown();
    return 0;
}