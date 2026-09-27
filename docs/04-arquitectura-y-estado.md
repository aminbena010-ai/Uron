# 04 — Arquitectura y estado

Visión global del motor: capas, carpetas, convenciones, reglas de Vulkan y
dónde está hoy el proyecto (qué funciona, qué es stub y qué está en el
roadmap).

---

## 1. Capas

```
Usuario (su app: Engine, Window, Scene2D, Shader, plugins...)
   │
   ▼
Uron::Engine          ← API pública, oculta Vulkan/GLFW (pimpl)
   ├── Window              GLFW oculto (pimpl), callbacks doble canal
   ├── Input               teclado/ratón por frame (alimentado por Window)
   ├── Scene2D             árbol de Node2D (update/render)
   ├── Renderer (interfaz) → VulkanRenderer (impl, core/src/vulkan/)
   │        ├── SpritePipeline     sprites 2D (push 48 bytes)
   │        ├── Pipeline           mallas 3D (mesh.vert/frag, push 64)
   │        └── VulkanShader       shaders custom (push 128 bytes)
   └── PluginManager       carga/actualiza/descarga plugins (IPlugin)
```

Regla transversal: **el usuario solo ve `Uron::*`**. Los tipos de Vulkan viven
en `core/src/vulkan/`, GLFW solo en `core/src/Window.cpp`, y ninguno aparece
en las cabeceras públicas.

---

## 2. Mapa de carpetas

```
Uron/
├── CLAUDE.md / claude.md      reglas del proyecto (leer antes de tocar)
├── map_project.txt            índice de TODOS los archivos
├── CMakeLists.txt             raíz: opciones + glslc de shaders
├── README.md                  presentación y roadmap
├── docs/                      esta documentación
├── core/
│   ├── include/Uron/          API pública (Types, Logger, Window, Input,
│   │   ├── math/                 Engine, render/, scene/, Plugin/)
│   └── src/                   implementaciones
│       ├── Plugin/            PluginManager, Registry, Context, ApiPlugin
│       ├── scene/             Node2D, Scene2D, Sprite2D
│       ├── render/            Texture (stb), Shader, ShaderInternal.h
│       └── vulkan/            PRIVADO: Context, Swapchain, pipelines...
├── plugins/                   plugins oficiales (ahora: physics/)
├── examples/                  hello_* (+ assets copiados a build/bin)
├── shaders/                   .vert/.frag → build/bin/shaders/*.spv
└── third_party/stb/           stb_image (implementation en Texture.cpp)
```

Tabla rápida (extraída de `map_project.txt`): "necesito X → abro Y"

| Necesito... | Archivo |
|---|---|
| Tipo público nuevo | `core/include/Uron/Types.h` |
| Clase pública nueva | `core/include/Uron/...` |
| Cambiar la interfaz de render | `render/Renderer.h` + `vulkan/VulkanRenderer.*` |
| Log | `Logger.h` (`URON_INFO/WARN/ERROR`) |
| Nodo/escena/sprite | `core/include/Uron/scene/*` |
| Plugin nuevo | `plugins/<nombre>/` + `URON_PLUGIN` |
| Ejemplo nuevo | `examples/<nombre>/` + `examples/CMakeLists.txt` |
| Shader nuevo | `shaders/<nombre>.{vert,frag}` (glslc automático) |
| Lógica Vulkan | `core/src/vulkan/*` — **nunca** en `scene/` ni `render/` |

---

## 3. Convenciones de código

- **C++17**, indentación 4 espacios, llaves Allman (en su propia línea).
- Nombres: clases `PascalCase`, métodos/variables `camelCase`, miembros
  `m_camelCase`, constantes `PascalCase`/`UPPER_SNAKE`, namespaces
  `PascalCase` (`Uron::Vulkan`).
- Punteros/referencias pegados al tipo: `Type* name`, `Type& name`.
- `#pragma once` siempre (nunca `#ifndef`).
- Orden de includes: cabecera propia → `<Uron/...>` → terceros → STL.
- **pimpl** cuando hay que ocultar detalles (`Window`, `Engine`, `Shader`,
  `Input`, `VulkanRenderer`).
- Comentarios: bloque explicativo en cabeceras públicas; en `.cpp` solo lo
  que no es obvio (decisiones de diseño, workarounds).
- Logs con `URON_INFO/WARN/ERROR/FATAL/TRACE`.
- `static_assert` cuando el layout importa (push constants, rangos de `Key`).

---

## 4. Reglas de build

- Todo `.cpp` nuevo → `URON_CORE_SOURCES` en `core/CMakeLists.txt`.
- Cabecera pública nueva → `URON_CORE_HEADERS`.
- Cabecera privada (`src/**`) → `URON_CORE_PRIVATE_HEADERS`.
- Ejemplo nuevo → bloque en `examples/CMakeLists.txt` (con
  `add_dependencies(... uron_example_data)`).
- Plugins → `plugins/<n>/CMakeLists.txt` + `add_subdirectory` en
  `plugins/CMakeLists.txt`.
- Shaders y assets se copian con `POST_BUILD` a `build/bin/`; los `.spv`
  los genera `glslc` (objetivo `uron_shaders`), **nunca** copiar `.spv` desde
  `shaders/` (pisa los recién compilados).
- Verificación mínima: `cmake --build build` → **0 errores, 0 warnings** y
  ejecutar el ejemplo más cercano al cambio.
- Versión sincronizada en tres sitios: `CMakeLists.txt` (`PROJECT_VERSION`),
  `Uron.h` (`URON_VERSION_*`) y el banner de `Engine.cpp`.

---

## 5. Reglas de Vulkan (resumen)

- Todo el código Vulkan está en `core/src/vulkan/`; el usuario jamás incluye
  `<vulkan/vulkan.h>`.
- Push constants:
  - **Sprites (por defecto):** 48 bytes = `vec4 row0 (a b c d)` +
    `vec4 row1 (e f screenW screenH)` + `vec4 tint`.
  - **Custom:** 128 bytes = 48 del sprite + 80 de uniforms (std430 desde
    offset 48), sin padding manual sin `static_assert`.
  - **Mallas 3D:** 64 bytes = `mat4 MVP` (VERTEX).
- Sincronización: `MAX_FRAMES = 2`; semáforos `imageAvailable[N]` por frame,
  `renderFinished[imageIndex]` **por imagen de swapchain**; vallas
  `inFlight[N]` con `VK_FENCE_CREATE_SIGNALED_BIT`.
- Validación: `VK_LAYER_KHRONOS_validation` + debug messenger (activa en Debug
  o con `URON_VALIDATION=1`); los ejemplos corren sin mensajes.
- La creación del pipeline custom es perezosa (primer `drawSprite` con ese
  `Shader::handle()`), cacheada, y si falla cae al pipeline de sprites.

---

## 6. Estado actual (Uron 0.4.71)

### ✅ Funciona

| Sistema | Notas |
|---|---|
| Core + build | CMake 3.20+, glslc automático, ejemplos en `build/bin/` |
| Window | GLFW oculto, config, fullscreen/vsync, callbacks doble canal |
| Engine | init/attach/loop/shutdown, delta/total, escenas, plugins |
| Input (fase E) | teclado + ratón, bordes por frame, scroll, `engine.input()` |
| Render 2D | sprites con textura, tinte, transformación por jerarquía, `Scene2D::setCamera` con `Camera2D::viewMatrix()` |
| Render 3D | `drawMesh` (MVP = viewProj·transform, depth D32, `MeshBuilder::cube`), `setViewProjection`, ejemplo `hello_cube` |
| Shaders custom | `.spv` + uniforms en runtime (fase D), 128 bytes push |
| Escenas | árbol `Node2D`, ciclo enter/update/exit/render, transform/tint compuestos padre→hijo, `Sprite2D` |
| Plugins | sistema completo (manager/registry/context/config/bus/services) + plugin Physics oficial |
| Texturas | PNG/JPG (stb_image), filter/wrap/mipmaps, desde memoria o archivo |
| Logger | niveles + macros |
| Math | Vec2/3/4, Mat3/4, Quat, utilidades |

### ⚠️ Conocido / stub a propósito

- `ApiPlugin`: callbacks `onUpdate/onRender/onEvent` no se invocan aún;
  `renderer()/scene2D()/loadTexture()/loadShader()` devuelven `nullptr`.
- `URON_BUILD_TESTS=ON` no tiene targets todavía.
- Escena 3D: pipeline + `drawMesh` + `hello_cube` listos; falta un grafo de
  escenas 3D propio e iluminación más allá del lambert direccional.
- Si la ventana pierde foco con una tecla mantenida, el estado puede quedar
  "hasta que GLFW entregue el release" (sin manejo de foco todavía).

### 🗺️ Roadmap (README)

- [x] Core, Vulkan, escenas 2D, sprites, shaders custom, input básico
- [ ] Plugin oficial: Input extendido (acciones, gamepad)
- [ ] Plugins: Audio, Lighting, Partículas, UI, Animación
- [ ] Escenas 3D e iluminación
- [ ] Editor visual
- [ ] Backend WebGPU

---

## 7. Workflow de desarrollo

Antes de codificar:

1. Lee `map_project.txt` → ¿ya existe algo parecido? (no duplicar)
2. Lee `claude.md` → reglas del proyecto.
3. Escribe el código siguiendo el estilo.
4. Actualiza `map_project.txt` si creas archivos (mismo commit).
5. Añade `.cpp`/`.h` al `CMakeLists.txt` correspondiente.
6. `cmake --build build` → 0 errores, 0 warnings.
7. Prueba el ejemplo más cercano al cambio.
8. Commit: `tipo(alcance): descripción`
   (`feat`, `fix`, `refactor`, `docs`, `build`, `chore`).

---

**Uron — Light, flexible, lethal.** 🦡
