#include <Uron/Uron.h>
#include <cmath>

int main() {
    Uron::Engine engine;
    if (!engine.init()) return -1;

    Uron::Window window;
    Uron::WindowConfig cfg;
    cfg.title  = "Hello Shader";
    cfg.width  = 1280;
    cfg.height = 720;

    if (!window.create(cfg)) return -1;
    if (!engine.attachWindow(&window)) return -1;

    Uron::Scene2D scene("ShaderScene");
    scene.setClearColor(Uron::Color::Black());

    Uron::Shader waveShader;
    if (!waveShader.loadFromFile("shaders/wave.vert.spv",
                                 "shaders/wave.frag.spv")) {
        return -1;
    }

    auto sprite = std::make_unique<Uron::Sprite2D>("Player");
    sprite->setTexture("assets/player.png");
    sprite->setSize({256.f, 256.f});
    sprite->setPosition({512.f, 360.f});
    sprite->setShader(&waveShader);

    scene.root()->addChild(std::move(sprite));

    engine.setScene(&scene);

    while (!window.shouldClose()) {
        window.pollEvents();

        // Orden = orden de declaracion del bloque push_constant del GLSL.
        const float t = static_cast<float>(engine.totalTime());
        waveShader.setUniform("time", t);
        waveShader.setUniform("amplitude", 8.f + 4.f * std::sin(t * 1.5f));
        waveShader.setUniform("tint", Uron::Color{1.f, 0.85f, 0.4f, 1.f});

        engine.beginFrame();
        engine.clear(Uron::Color::Black());
        engine.endFrame();
    }

    engine.setScene(nullptr);
    engine.shutdown();
    window.destroy();
    return 0;
}