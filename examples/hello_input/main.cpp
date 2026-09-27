#include <Uron/Uron.h>
#include <cstdio>

int main() {
    Uron::Engine engine;
    if (!engine.init()) return -1;

    Uron::Window window;
    Uron::WindowConfig cfg;
    cfg.title  = "Hello Input";
    cfg.width  = 1280;
    cfg.height = 720;

    if (!window.create(cfg)) return -1;
    if (!engine.attachWindow(&window)) return -1;

    Uron::Scene2D scene("InputScene");
    scene.setClearColor(Uron::Color::Black());

    auto sprite = std::make_unique<Uron::Sprite2D>("Player");
    sprite->setTexture("assets/player.png");
    sprite->setSize({256.f, 256.f});
    sprite->setPosition({512.f, 232.f});

    auto* player = sprite.get();
    scene.root()->addChild(std::move(sprite));

    engine.setScene(&scene);

    URON_INFO("WASD/flechas: mover | RMB: arrastrar | scroll: escalar");
    URON_INFO("Espacio: log de pressed | Escape: salir");

    while (!window.shouldClose()) {
        window.pollEvents();
        engine.beginFrame();
        engine.clear(Uron::Color::Black());

        const auto& in  = engine.input();
        const float speed = 400.f * engine.deltaTime();

        Uron::Vec2 move{0.f, 0.f};
        if (in.isKeyDown(Uron::Key::W) || in.isKeyDown(Uron::Key::Up))    move.y -= 1.f;
        if (in.isKeyDown(Uron::Key::S) || in.isKeyDown(Uron::Key::Down))  move.y += 1.f;
        if (in.isKeyDown(Uron::Key::A) || in.isKeyDown(Uron::Key::Left))  move.x -= 1.f;
        if (in.isKeyDown(Uron::Key::D) || in.isKeyDown(Uron::Key::Right)) move.x += 1.f;
        if (move.lengthSq() > 0.f) {
            player->translate(move.normalized() * speed);
        }

        if (in.isMousePressed(Uron::MouseButton::Left)) {
            const Uron::Vec2 p = in.mousePosition();
            char buf[96];
            std::snprintf(buf, sizeof(buf), "Click en (%.0f, %.0f)",
                          p.x, p.y);
            URON_INFO(buf);
        }

        if (in.isMouseDown(Uron::MouseButton::Right)) {
            player->translate(in.mouseDelta());
        }

        const Uron::Vec2 scroll = in.scrollDelta();
        if (scroll.y != 0.f) {
            player->setScale(player->scale() * (1.f + scroll.y * 0.1f));
        }

        if (in.isKeyPressed(Uron::Key::Space)) {
            URON_INFO("Espacio (borde pressed)");
        }
        if (in.isKeyPressed(Uron::Key::Escape)) {
            window.requestClose();
        }

        engine.endFrame();
    }

    engine.setScene(nullptr);
    engine.shutdown();
    window.destroy();
    return 0;
}
