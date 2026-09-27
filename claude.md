# CLAUDE.md — Uron project rules

This file defines the rules Claude must follow when working on the
**Uron** engine. **Read it before touching any file.**

---

## 0. Golden rule

> **Before writing any code, check `map_project.txt` to know which files
> exist, where they are, and what they do. Never create new files
> without first checking that they don't already exist.**

If something is not in `map_project.txt`, it probably doesn't exist.
If you are going to create something new, update `map_project.txt`
**in the same commit**.

---

## 1. Project identity

- **Name:** Uron
- **Type:** 3D game engine (with 2D support)
- **Philosophy:** Minimal core (render only) + plugins for everything else.
- **Language:** C++17
- **Graphics:** Vulkan 1.2+
- **Window:** GLFW (hidden from the user)
- **Build:** CMake 3.20+
- **Tagline:** "Light, flexible, lethal."
- **Mascot:** Ferret

---

## 2. Overall architecture

```
User
  │
  ▼
Uron::Engine  (public API, hides Vulkan)
  │
  ├── Window              (GLFW underneath)
  ├── Scene2D             (hierarchical nodes)
  ├── Renderer (interface) → VulkanRenderer (impl)
  └── PluginManager       (official and community plugins)
```

**Rule:** The user **never** sees Vulkan types, GLFW, or unusual STL in
the public API. They only see `Uron::Engine`, `Uron::Window`,
`Uron::Color`, `Uron::Texture`, `Uron::Shader`, `Uron::Sprite2D`, etc.

---

## 3. Folder structure

```
Uron/
├── CLAUDE.md                ← this file
├── map_project.txt          ← file index
├── CMakeLists.txt
├── README.md
├── LICENSE
├── core/                    ← base engine
│   ├── CMakeLists.txt
│   ├── include/Uron/        ← public headers
│   └── src/                 ← implementations
├── plugins/                 ← official plugins
├── examples/                ← usage examples
├── shaders/                 ← GLSL + .spv
└── third_party/             ← stb_image, etc.
```

**Rule:** Public headers go in `core/include/Uron/...`. Implementations
go in `core/src/...`. **Never** mix public and private headers.

---

## 4. Code rules

### 4.1 Style

- **Standard:** C++17.
- **Indentation:** 4 spaces, no tabs.
- **Braces:** Allman style (brace on its own line).
- **Names:**
  - Classes: `PascalCase` → `Sprite2D`, `VulkanRenderer`.
  - Methods: `camelCase` → `drawSprite`, `setPosition`.
  - Member variables: `m_` + camelCase → `m_size`, `m_texture`.
  - Constants: `PascalCase` or `UPPER_SNAKE` → `Color::Black()`.
  - Namespaces: `PascalCase` → `Uron::Vulkan`.
- **Pointers:** `Type* name` (asterisk attached to the type).
- **References:** `Type& name` (ampersand attached to the type).

### 4.2 Comments

- **In public headers:** header block explaining what it is, what it
  contains, what it's for, who uses it.
- **In implementations (.cpp):** minimal. Only comment where something
  is not obvious.

### 4.3 Includes

- Always `#pragma once`, never `#ifndef`.
- Order:
  1. Own header of the file (if applicable).
  2. Engine headers (`<Uron/...>`).
  3. Third-party headers (`<vulkan/...>`, `<GLFW/...>`).
  4. STL headers.
- Example:
  ```cpp
  #include "VulkanRenderer.h"
  #include <Uron/Logger.h>
  #include <vulkan/vulkan.h>
  #include <vector>
  ```

### 4.4 No unnecessary comments

- **Do not** comment the obvious: `// increment i` ← forbidden.
- **Do** comment design decisions, hacks, workarounds.

---

## 5. Vulkan-specific rules

- **All** Vulkan code lives in `core/src/vulkan/`.
- The user **never** includes `<vulkan/vulkan.h>` directly.
- Vulkan types (`VkDevice`, `VkPipeline`, etc.) are hidden with pimpl
  or `void*` in public headers.
- `.spv` files are generated with `glslc` from `.vert` / `.frag` in
  `shaders/`.

### Push constant layout rules

- **SpritePipeline:** `sizeof(float) * 12` = 48 bytes.
  ```
  offset  0: vec4 row0   (a b c d  — matriz afín 2D)
  offset 16: vec4 row1   (e f screenW screenH)
  offset 32: vec4 spriteTint
  ```
- **Custom shaders:** `sizeof(float) * 32` = 128 bytes (48 de sprite +
  80 de uniforms, `UNIFORM_PUSH_BYTES`, alineados std430 desde offset 48).
- **Never** add manual padding without a `static_assert` verifying it.

### Depth rules

- El render pass **siempre** tiene un attachment `D32_SFLOAT`; todo
  pipeline declara `VkPipelineDepthStencilStateCreateInfo` (sprites:
  test/write OFF; mallas: test ON, write ON, LESS_OR_EQUAL).

### Sync rules

- 2 frames in flight (MAX_FRAMES = 2).
- Semaphores: `imageAvailable[N]`, `renderFinished[imageIndex]`
  (uno por imagen de swapchain: reutilizar por frame hacia que el
  present anterior siguiera usando el semáforo).
- Fences: `inFlight[N]`, con `VK_FENCE_CREATE_SIGNALED_BIT`.
- Validación: activa por defecto en Debug; `URON_VALIDATION=0/1`
  la fuerza a OFF/ON (también funciona en Release).

---

## 6. Plugin system

### Mandatory standard for every plugin

1. Namespace `Uron::Plugin::Name`.
2. Inherits from `Uron::Plugin::IPlugin`.
3. Defines `ID`, `NAME`, `VERSION`, `AUTHOR` (constexpr).
4. Implements `onLoad`, `onUnload`, `onUpdate`.
5. Registers with `URON_PLUGIN(NS, CLASS)`.
6. One plugin = one responsibility.
7. No direct Vulkan access.
8. No global state.

### How the user uses it

```cpp
engine.importPlugin<Uron::Plugin::Physics::Plugin>();
auto& physics = engine.plugin<Uron::Plugin::Physics::Plugin>();
```

**Rule:** The user **never** imports the plugin's `.h` as an external
library. The engine provides it internally.

---

## 7. Build rules (CMake)

- **Every** new `.cpp` must be added to `URON_CORE_SOURCES` in
  `core/CMakeLists.txt`.
- **Every** new public header must be added to `URON_CORE_HEADERS`.
- **Every** private header (`src/vulkan/*.h`) must be added to
  `URON_CORE_PRIVATE_HEADERS`.
- **Every** new example is added to `examples/CMakeLists.txt`.
- Shaders are copied to `build/bin/shaders/` via `POST_BUILD`.
- Assets are copied to `build/bin/assets/` via `POST_BUILD`.

---

## 8. Commit naming

Format: `type(scope): description`

Valid types:
- `feat` → new feature.
- `fix` → bug fix.
- `refactor` → restructure without functional change.
- `docs` → documentation only.
- `build` → CMake or build changes.
- `chore` → minor tasks.

Examples:
```
feat(render): add Shader class with pimpl
fix(vulkan): fix push constant size mismatch
docs(claude): add rules for custom shaders
build(cmake): add hello_shader example
```

---

## 9. Workflow

Before coding:

1. **Read `map_project.txt`** to locate existing files.
2. **Check** whether something similar already exists (do not duplicate).
3. **Check** this `CLAUDE.md` for specific rules.
4. **Write** code following the project style.
5. **Update** `map_project.txt` if you created new files.
6. **Add** the `.cpp` and `.h` to the corresponding `CMakeLists.txt`.
7. **Build** with `cmake --build build` and verify there are no errors.
8. **Test** with the simplest existing example.

---

## 10. Things to NEVER do

- ❌ Expose Vulkan types in public headers.
- ❌ Expose GLFW in public headers.
- ❌ Create files without updating `map_project.txt`.
- ❌ Duplicate code that already exists in `core/`.
- ❌ Put game logic in the engine (that's plugins).
- ❌ Put Vulkan logic in `scene/` or `render/`.
- ❌ Add external dependencies without justification.
- ❌ Break the public API without updating `Uron.h`.
- ❌ Leave warnings unresolved.
- ❌ Ignore include order.

---

## 11. Things to ALWAYS do

- ✅ Check `map_project.txt` before creating files.
- ✅ Use `URON_INFO`, `URON_WARN`, `URON_ERROR` for logs.
- ✅ Use pimpl when internal details must be hidden.
- ✅ Add `static_assert` when layout matters (push constants).
- ✅ Copy shaders and assets with `POST_BUILD` in CMake.
- ✅ Update `map_project.txt` **in the same commit**.
- ✅ Build before saying "done".
- ✅ Test the example closest to the change.

---

## 12. Quick reference

| I need to... | Go to... |
|---|---|
| Add a public type | `core/include/Uron/Types.h` |
| Add a public class | `core/include/Uron/...` |
| Implement Vulkan | `core/src/vulkan/...` |
| Add a scene node | `core/include/Uron/scene/` |
| Create a plugin | `plugins/` + `URON_PLUGIN` macro |
| Add an example | `examples/` + `examples/CMakeLists.txt` |
| Add a shader | `shaders/*.vert` + `*.frag` + `glslc` |
| Add an asset | `examples/<name>/assets/` |
| Update the index | `map_project.txt` |

---

**Uron — Light, flexible, lethal.** 🦡


---

## Note on `map_project.txt`

The `map_project.txt` I gave you is already in English, so **no changes needed** there. If you want, I can also give you an English version with the same structure just to keep it 100% consistent. Just say the word.

---



