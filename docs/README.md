# Documentación de Uron

> **Uron** — *Ligero, flexible, letal.* 🦡
> Motor de videojuegos C++17 con backend Vulkan, core mínimo y plugins.

Esta carpeta contiene la documentación de uso y diseño del motor. El índice
`map_project.txt` (raíz del proyecto) sigue siendo el mapa de archivos y
`claude.md` las reglas de desarrollo.

---

## Índice

| Doc | Contenido |
|---|---|
| [01 — Uso del motor](01-uso-del-motor.md) | Requisitos, build, ciclo de vida, ventana, bucle, **input**, logger, matemáticas, ejemplos. |
| [02 — Escena y render](02-escena-y-render.md) | `Scene2D`, `Node2D`, `Sprite2D`, `Camera2D`, texturas, `Renderer`, **shaders custom**, push constants y `drawMesh` 3D. |
| [03 — Plugins](03-plugins.md) | Cómo **usar** plugins (`importPlugin`), cómo **crear** uno paso a paso, contexto, eventos, servicios y configuración. |
| [04 — Arquitectura y estado](04-arquitectura-y-estado.md) | Capas del motor, carpetas, convenciones de código, reglas Vulkan, estado actual conocido y roadmap. |

---

## Arranque en 30 segundos

```bash
# dependencias (Debian/Ubuntu)
sudo apt install cmake build-essential libglfw3-dev libvulkan-dev glslang-tools

# compilar (los .spv de shaders se generan solos con glslc)
cmake -S . -B build
cmake --build build -j

# ejecutar un ejemplo (los binarios quedan en build/bin/)
cd build/bin && ./hello_uron
```

```cpp
#include <Uron/Uron.h>

int main() {
    Uron::Engine engine;
    engine.init();

    Uron::Window window;
    window.create({.title = "Mi Juego", .width = 1280, .height = 720});
    engine.attachWindow(&window);          // crea Vulkan + cablea el input

    while (!window.shouldClose()) {
        window.pollEvents();               // 1. eventos de SO
        engine.beginFrame();               // 2. update (escena, plugins, input)
        engine.clear(Uron::Color::Black());// 3. fondo
        engine.endFrame();                 // 4. dibuja + presenta
    }

    window.destroy();
    engine.shutdown();
    return 0;
}
```

---

## Regla de oro

- El usuario **nunca** ve tipos de Vulkan ni GLFW: solo `Uron::*`.
- El core trae **ventana, render, escenas, input y plugins**; todo lo demás
  (físicas, audio, animación...) son **plugins**.
- Antes de tocar código: lee `claude.md` (reglas) y `map_project.txt` (qué
  existe y dónde).
