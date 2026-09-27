#include <Uron/Uron.h>
#include <cmath>
#include <cstdio>

class Player : public Uron::Node2D {
public:
    Player() : Uron::Node2D("Player") {}

    void onEnter(Uron::Scene2D& scene) override {
        (void)scene;
        URON_INFO("Player entro a la escena");
    }

    void onUpdate(Uron::Scene2D& scene, float dt) override {
        (void)scene;
        m_time += dt;
        setPosition({
            640.f + 300.f * std::sin(m_time * 1.5f),
            360.f + 150.f * std::cos(m_time * 2.3f)
        });
    }

    void onExit(Uron::Scene2D& scene) override {
        (void)scene;
        URON_INFO("Player salio de la escena");
    }

private:
    float m_time = 0.f;
};

class Rotator : public Uron::Node2D {
public:
    Rotator() : Uron::Node2D("Rotator") {}

    void onUpdate(Uron::Scene2D& scene, float dt) override {
        (void)scene;
        rotate(dt);
    }
};

int main() {
    Uron::Engine engine;
    if (!engine.init()) return -1;

    Uron::Window window;
    Uron::WindowConfig cfg;
    cfg.title  = "Hello Scene";
    cfg.width  = 1280;
    cfg.height = 720;

    if (!window.create(cfg)) return -1;
    if (!engine.attachWindow(&window)) return -1;

    Uron::Scene2D scene("MainScene");
    scene.setClearColor(Uron::Color::Black());

    auto player  = std::make_unique<Player>();
    auto rotator = std::make_unique<Rotator>();

    player->setPosition({640.f, 360.f});
    rotator->setPosition({100.f, 100.f});

    player->addChild(std::move(rotator));

    scene.root()->addChild(std::move(player));

    auto camera = std::make_unique<Uron::Camera2D>("MainCamera");
    camera->setViewportSize(1280.f, 720.f);
    camera->setPosition({640.f, 360.f});   // centro de la pantalla => vista identidad
    scene.setCamera(camera.get());
    scene.root()->addChild(std::move(camera));

    engine.setScene(&scene);

    float timer = 0.f;

    while (!window.shouldClose()) {
        window.pollEvents();
        engine.beginFrame();
        engine.clear(Uron::Color::Black());

        timer += engine.deltaTime();
        if (timer > 1.f) {
            auto* p = dynamic_cast<Player*>(scene.root()->children()[0].get());
            if (p) {
                char buf[128];
                std::snprintf(buf, sizeof(buf),
                    "Player: (%.1f, %.1f)  rot=%.2f",
                    p->position().x, p->position().y, p->rotation());
                URON_INFO(buf);
            }
            timer = 0.f;
        }

        engine.endFrame();
    }

    engine.setScene(nullptr);
    engine.shutdown();
    window.destroy();
    return 0;
}