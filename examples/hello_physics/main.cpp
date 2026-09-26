#include <Uron/Uron.h>
#include <Uron/Plugin/Physics.h>

#include <cstdio>

int main() {
    Uron::Engine engine;
    if (!engine.init()) return -1;

    Uron::Window window;
    Uron::WindowConfig cfg;
    cfg.title  = "Hello Physics";
    cfg.width  = 1280;
    cfg.height = 720;

    if (!window.create(cfg)) return -1;
    if (!engine.attachWindow(&window)) return -1;

    engine.importPlugin<Uron::Plugin::Physics::Plugin>();

    auto& physics = engine.plugin<Uron::Plugin::Physics::Plugin>();
    physics.setGravity({0.f, -9.8f, 0.f});

    Uron::Plugin::Physics::BodyDesc desc;
    desc.position    = {0.f, 5.f, 0.f};
    desc.mass        = 1.f;
    desc.restitution = 0.6f;

    auto bodyId = physics.createBody(desc);

    float timer = 0.f;
    while (!window.shouldClose()) {
        window.pollEvents();
        engine.beginFrame();
        engine.clear(Uron::Color::Black());

        auto* body = physics.getBody(bodyId);
        if (body) {
            timer += engine.deltaTime();
            if (timer > 0.5f) {
                char buf[128];
                std::snprintf(buf, sizeof(buf),
                    "Body %u -> y = %.3f",
                    body->id, body->position.y);
                URON_INFO(buf);
                timer = 0.f;
            }
        }

        engine.endFrame();
    }

    engine.shutdown();
    window.destroy();
    return 0;
}