#include <Uron/Uron.h>
#include <Uron/render/Mesh.h>
#include <cmath>
#include <memory>

// Nodo que dibuja una malla 3D con drawMesh (BUG-001): el MVP se arma en
// onRender y la rotacion avanza con el dt de la escena.
class CubeNode : public Uron::Node2D {
public:
    CubeNode() : Uron::Node2D("Cube") {}

    void onUpdate(Uron::Scene2D& scene, float dt) override {
        (void)scene;
        m_time += dt;
    }

    void onRender(Uron::Renderer& renderer) override {
        float aspect = static_cast<float>(renderer.width()) /
                       static_cast<float>(renderer.height());
        const float kFov = 45.f * 3.14159265f / 180.f;

        Uron::Mat4 proj  = Uron::Mat4::perspective(kFov, aspect, 0.1f, 100.f);
        Uron::Mat4 view  = Uron::Mat4::translation({0.f, 0.f, -3.f});
        Uron::Mat4 model = Uron::Mat4::rotationY(m_time) *
                           Uron::Mat4::rotationX(m_time * 0.6f);

        renderer.setViewProjection(proj * view);
        renderer.drawMesh(m_cube, model);
    }

private:
    Uron::Mesh m_cube = Uron::MeshBuilder::cube(1.f);
    float m_time = 0.f;
};

int main() {
    Uron::Engine engine;
    if (!engine.init()) return -1;

    Uron::Window window;
    Uron::WindowConfig cfg;
    cfg.title  = "Hello Cube";
    cfg.width  = 1280;
    cfg.height = 720;

    if (!window.create(cfg)) return -1;
    if (!engine.attachWindow(&window)) return -1;

    Uron::Scene2D scene("CubeScene");
    scene.setClearColor(Uron::Color::Black());
    scene.root()->addChild(std::make_unique<CubeNode>());
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
