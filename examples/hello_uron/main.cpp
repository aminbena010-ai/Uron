#include <Uron/Uron.h>
#include <GLFW/glfw3.h>
#include <cstdio>

int main() {
    Uron::Logger::setLevel(Uron::LogLevel::Info);

    // 1. Ventana
    Uron::Window window;
    Uron::WindowConfig cfg;
    cfg.title  = "Cubo girando (2D)";
    cfg.width  = 1280;
    cfg.height = 720;
    if (!window.create(cfg)) return -1;

    Uron::WindowCallbacks cbs;
    cbs.onClose = [&window]() { window.requestClose(); };
    window.setCallbacks(cbs);

    // 2. Motor
    Uron::Engine engine;
    if (!engine.init()) return -1;
    if (!engine.attachWindow(&window)) return -1;

    // 3. Escena
    Uron::Scene2D scene("Spin");
    scene.setClearColor(Uron::Color::fromHex(0x0a0a10));
    engine.setScene(&scene);

    // 4. Cuadrado (sprite con textura blanca 1x1 dentro del propio sprite)
    auto quad = std::make_unique<Uron::Sprite2D>("Quad");
    quad->setSize({200.f, 200.f});
    quad->setPosition({0.f, 0.f});
    const Uron::u8 px[4] = { 255, 255, 255, 255 };
    quad->texture().loadFromMemory(px, 1, 1);   // blanco: el color sale del shader

    // 5. Shader custom de rotación
    Uron::Shader spinShader;
    if (!spinShader.loadFromFile("shaders/spin.vert.spv",
                                 "shaders/spin.frag.spv")) {
        URON_ERROR("No se pudo cargar spin shader");
        return -1;
    }

    // ⚠️ El orden de setUniform importa: es el orden en el push constant.
    // Con un solo uniform ("time"), el primero que se llame define el offset.
    spinShader.setUniform("time", 0.f);

    quad->setShader(&spinShader);
    scene.root()->addChild(std::move(quad));

    // 6. Loop
    float t = 0.f;
    while (!window.shouldClose()) {
        window.pollEvents();

        engine.beginFrame();
        engine.clear(scene.clearColor());

        t += engine.deltaTime();
        spinShader.setUniform("time", t);

        engine.endFrame();
    }

    engine.shutdown();
    window.destroy();
    return 0;
}