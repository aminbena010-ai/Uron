#include <Uron/Uron.h>

int main() {
    Uron::Engine engine;
    if (!engine.init()) return -1;

    Uron::Window window;
    Uron::WindowConfig cfg;
    cfg.title  = "Hello Sprite";
    cfg.width  = 1280;
    cfg.height = 720;

    if (!window.create(cfg)) return -1;
    if (!engine.attachWindow(&window)) return -1;

    Uron::Scene2D scene("SpriteScene");
    scene.setClearColor(Uron::Color::Black());

    auto sprite = std::make_unique<Uron::Sprite2D>("Player");
    sprite->setTexture("assets/player.png");
    sprite->setSize({256.f, 256.f});
    sprite->setPosition({320.f, 180.f});

    scene.root()->addChild(std::move(sprite));

    engine.setScene(&scene);

    while (!window.shouldClose()) {
        window.pollEvents();
        engine.beginFrame();
        engine.clear(Uron::Color::Black());
        engine.endFrame();
    }

    engine.setScene(nullptr);
    engine.shutdown();
    window.destroy();
    return 0;
}