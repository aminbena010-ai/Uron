# Uron Engine

> **Ligero, flexible, letal.**

Uron es un motor de videojuegos **3D (con soporte 2D)** escrito en C++ sobre **Vulkan**.

Su nombre viene del **hurón**: un animal pequeño, rapidísimo y que se adapta a cualquier espacio. Igual que Uron: **core mínimo, plugins para todo lo demás**.

---

## Filosofía

- El **core** solo trae render (ventana, Vulkan, escenas, sistema de plugins).
- **Todo lo demás son plugins**: físicas, iluminación, audio, input, animación...
- El usuario importa el motor como **librería** y enchufa solo lo que necesita.
- El motor **no asume nada**. El usuario controla todo: ventana, loop, plugins.

---

## Características

| Sistema | En core | Como plugin |
|---|---|---|
| Ventana | ✅ | — |
| Render Vulkan 2D/3D | ✅ | — |
| Sistema de plugins | ✅ | — |
| Logger | ✅ | — |
| Matemáticas (Vec2/3/4, Mat3/4, Quat) | ✅ | — |
| Físicas | — | ✅ |
| Iluminación avanzada | — | ✅ |
| Audio | — | ✅ |
| Input extendido | — | ✅ |
| Animación | — | ✅ |
| Partículas | — | ✅ |
| UI | — | ✅ |

---

## Ejemplo rápido

```cpp
#include <Uron/Uron.h>

int main() {
    Uron::Engine engine;
    engine.init();

    Uron::Window window;
    Uron::WindowConfig cfg;
    cfg.title  = "Mi Juego";
    cfg.width  = 1280;
    cfg.height = 720;

    if (!window.create(cfg)) return -1;

    engine.attachWindow(&window);

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
```

---

## Plugins

Todo plugin de Uron vive en `Uron::Plugin::X` y sigue un **estándar fijo**:

```cpp
#include <Uron/Uron.h>
#include <Uron/Plugin/Physics.h>

int main() {
    Uron::Engine engine;
    engine.init();

    Uron::Window window;
    window.create({.title = "Mi Juego"});
    engine.attachWindow(&window);

    engine.importPlugin<Uron::Plugin::Physics::Plugin>();

    auto& physics = engine.plugin<Uron::Plugin::Physics::Plugin>();
    physics.setGravity({0.f, -9.8f, 0.f});

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
```

### Estándar de plugins

1. Namespace `Uron::Plugin`
2. Hereda de `IPlugin`
3. Metadatos `NAME`, `VERSION`, `AUTHOR`
4. Ciclo de vida `onLoad`, `onUnload`, `onUpdate`
5. Registro con `URON_PLUGIN(NS, CLASS)`
6. Un plugin = una responsabilidad
7. Sin acceso directo a Vulkan
8. Sin estado global

---

## Estructura del proyecto

```
Uron/
├── core/               Motor base (solo render + plugins)
│   ├── include/Uron/   Headers públicos
│   └── src/            Implementación
│       ├── Plugin/     Sistema de plugins
│       └── vulkan/     Backend Vulkan
├── plugins/            Plugins oficiales
├── examples/           Ejemplos de uso
├── templates/          Plantilla para crear plugins
└── docs/               Documentación
```

---

## Requisitos

- **C++17** o superior
- **CMake** 3.20+
- **Vulkan SDK** 1.2+
- **GLFW** 3.3+

### Instalación rápida

**Linux (Debian/Ubuntu):**
```bash
sudo apt install cmake build-essential libglfw3-dev libvulkan-dev
```

**Windows (vcpkg):**
```bash
vcpkg install glfw3 vulkan
```

**macOS (Homebrew):**
```bash
brew install cmake glfw vulkan-sdk
```

---

## Compilar

```bash
git clone https://github.com/tu-usuario/Uron.git
cd Uron
cmake -B build
cmake --build build
```

Ejecutar ejemplo:
```bash
./build/bin/hello_uron
```

---

## Roadmap

- [x] Nombre, identidad y filosofía
- [x] Estructura del proyecto
- [x] Sistema de plugins (headers)
- [x] Core: Engine, Window, Logger
- [x] Backend Vulkan (init, swapchain, pipeline)
- [ ] Shaders por defecto (triángulo)
- [ ] Escenas 2D/3D
- [ ] Sprites y texturas
- [ ] Plugin oficial: Input
- [ ] Plugin oficial: Physics
- [ ] Plugin oficial: Audio
- [ ] Plugin oficial: Lighting
- [ ] Editor visual
- [ ] Backend WebGPU (web)

---

## Licencia

MIT — ver [LICENSE](LICENSE).

---

**Uron Engine** — *Ligero, flexible, letal.*