# 03 — Plugins

Los plugins son la forma de extender Uron **sin tocar el core**: físicas,
audio, iluminación, input extendido, animación... Todo lo que no es
"ventana + render + escena" es un plugin.

Este documento cubre dos cosas: **cómo usar** un plugin oficial y
**cómo crear** uno nuevo.

---

## 1. Filosofía

- Core mínimo: ventana, Vulkan, escenas, input y el **sistema de plugins**.
- Un plugin = **una responsabilidad**.
- El usuario importa el motor como librería y **enchufa solo lo que necesita**.
- El plugin **nunca** ve Vulkan, **nunca** arrastra estado global, y su API
  pública no expone GLFW ni STL raro.

```
engine.importPlugin<Uron::Plugin::Physics::Plugin>();
auto& physics = engine.plugin<Uron::Plugin::Physics::Plugin>();
```

---

## 2. Usar un plugin

Cabeceras oficiales en `core/include/Uron/Plugin/` (el usuario **no** añade
rutas externas: el motor lo trae integrado).

```cpp
#include <Uron/Uron.h>
#include <Uron/Plugin/Physics.h>

// 1. Importar (llama a onLoad; si falla, devuelve false)
engine.importPlugin<Uron::Plugin::Physics::Plugin>();

// 2. Usar su API tipada
auto& physics = engine.plugin<Uron::Plugin::Physics::Plugin>();
physics.setGravity({0.f, -9.8f, 0.f});
auto id = physics.createBody(desc);

// 3. Consultas
bool ok  = engine.isPluginLoaded("Physics");        // por nombre
auto* p  = engine.tryPlugin<Uron::Plugin::Physics::Plugin>();  // nullptr si no está
```

| Método | Comportamiento |
|---|---|
| `importPlugin<T>()` | Crea la instancia y llama `onLoad`. Duplicado → `URON_WARN` y `true`. |
| `plugin<T>()` | Referencia al plugin. **Solo si lo importaste** (si no, es UB: usa `tryPlugin`). |
| `tryPlugin<T>()` | Puntero o `nullptr`. |
| `isPluginLoaded(name)` | Por nombre (`NAME` del plugin). |
| `isPluginLoaded(PluginID)` | Por ID (`T::ID`); más rápido que por nombre. |

### Ciclo de vida

```
importPlugin<T>()  →  onLoad(ctx)          // una vez
beginFrame()       →  onUpdate(ctx, dt)    // cada frame (orden NO garantizado)
shutdown()         →  onUnload(ctx)        // una vez, al apagar el motor
```

Ejemplo completo: `examples/hello_physics/main.cpp`.

---

## 3. Estándar de todo plugin

Reglas obligatorias (ver `claude.md` §6):

1. Namespace `Uron::Plugin::<Nombre>`.
2. Hereda de `Uron::Plugin::IPlugin`.
3. Metadatos `ID`, `NAME`, `VERSION`, `AUTHOR` (`constexpr`).
4. Implementa `onLoad`, `onUnload`, `onUpdate`.
5. Se registra con `URON_PLUGIN(NS, CLASS)`.
6. Un plugin = una responsabilidad.
7. **Sin acceso directo a Vulkan.**
8. **Sin estado global.**

---

## 4. Crear un plugin paso a paso

### 4.1 Estructura de carpetas

```
plugins/
└── miplugin/
    ├── CMakeLists.txt
    ├── include/Uron/Plugin/MiPlugin.h    ← API pública (la ve el usuario)
    └── src/MiPlugin.cpp                  ← implementación
```

### 4.2 La cabecera

```cpp
// include/Uron/Plugin/MiPlugin.h
#pragma once
#include <Uron/Uron.h>
#include <Uron/Plugin/IPlugin.h>
#include <Uron/Plugin/PluginID.h>
#include <Uron/Plugin/Macros.h>

namespace Uron::Plugin::MiPlugin {

class Plugin final : public IPlugin {
public:
    static constexpr PluginID    ID      = makePluginID("MiPlugin");
    static constexpr const char* NAME    = "MiPlugin";
    static constexpr const char* VERSION = "1.0.0";
    static constexpr const char* AUTHOR  = "Tu Nombre";

    PluginID    id()      const override { return ID; }
    const char* name()    const override { return NAME; }
    const char* version() const override { return VERSION; }
    const char* author()  const override { return AUTHOR; }

    bool onLoad(PluginContext& ctx)   override;
    void onUnload(PluginContext& ctx) override;
    void onUpdate(PluginContext& ctx, float dt) override;

    // --- API publica del plugin (lo que usara el juego) ---
    void  setSpeed(float s) { m_speed = s; }
    float speed() const     { return m_speed; }

private:
    PluginContext* m_ctx = nullptr;
    float          m_speed = 1.f;
};

}

// Auto-registro en el PluginRegistry (estandar, regla 5)
URON_PLUGIN(MiPlugin, Plugin)
```

### 4.3 La implementación

```cpp
// src/MiPlugin.cpp
#include <Uron/Plugin/MiPlugin.h>

namespace Uron::Plugin::MiPlugin {

bool Plugin::onLoad(PluginContext& ctx) {
    m_ctx = &ctx;
    ctx.api().logInfo("MiPlugin loaded");
    // ctx.engine() → acceso al Engine (window, input, scene, deltaTime...)
    return true;
}

void Plugin::onUnload(PluginContext& ctx) {
    (void)ctx;
    ctx.api().logInfo("MiPlugin unloaded");
    m_ctx = nullptr;
}

void Plugin::onUpdate(PluginContext& ctx, float dt) {
    (void)ctx;
    // logica del plugin, una vez por frame
    m_speed += dt;
}

}
```

`IPlugin` también declara `onEvent(ctx, event)` con implementación vacía por
defecto: puedes sobreescribirla si manejas eventos del `EventBus`.

### 4.4 CMake

`plugins/miplugin/CMakeLists.txt` (plantilla: el de physics):

```cmake
add_library(uron_plugin_miplugin STATIC
    src/MiPlugin.cpp
)

add_library(Uron::Plugin::MiPlugin ALIAS uron_plugin_miplugin)

target_include_directories(uron_plugin_miplugin
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(uron_plugin_miplugin PUBLIC Uron::Core)

target_compile_features(uron_plugin_miplugin PUBLIC cxx_std_17)

set_target_properties(uron_plugin_miplugin PROPERTIES
    OUTPUT_NAME "uron_plugin_miplugin"
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
    target_compile_options(uron_plugin_miplugin PRIVATE /W4 /permissive-)
else()
    target_compile_options(uron_plugin_miplugin PRIVATE -Wall -Wextra -Wpedantic)
endif()
```

Registrar el directorio en `plugins/CMakeLists.txt`:

```cmake
add_subdirectory(physics)
add_subdirectory(miplugin)     # ← nueva linea
```

Y, si quieres un ejemplo, en `examples/CMakeLists.txt`:

```cmake
add_executable(hello_miplugin hello_miplugin/main.cpp)
target_link_libraries(hello_miplugin PRIVATE
    Uron::Core
    Uron::Plugin::MiPlugin      # ← alias del plugin
)
set_target_properties(hello_miplugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
add_dependencies(hello_miplugin uron_example_data)
```

Todo esto vive bajo las opciones `URON_BUILD_PLUGINS` y `URON_BUILD_EXAMPLES`
del CMake raíz.

### 4.5 La macro `URON_PLUGIN`

```cpp
URON_PLUGIN(MiPlugin, Plugin)
```

Registra una **factory** en el singleton `PluginRegistry`
(`registerFactory(makePluginID("MiPlugin.Plugin"), []{ return new Plugin(); })`)
mediante un objeto estático auto-ejecutable.

**Estado actual:** `engine.importPlugin<T>()` crea la instancia directamente
con `new T()` (por tipo), así que el macro **no es imprescindible** para
importar; forma parte del estándar y alimenta el `PluginRegistry`
(`create`, `exists`, `list`) por si se quiere crear plugins por ID sin
conocer el tipo en tiempo de compilación. `URON_PLUGIN_ID(NS, CLASS)` te da
el mismo ID que la macro.

---

## 5. PluginContext — lo que el motor te inyecta

Cada plugin recibe su propio `PluginContext&` en `onLoad/onUpdate/onUnload`:

| Acceso | Qué da |
|---|---|
| `ctx.engine()` | El `Engine` completo: `window()`, `input()`, `scene()`, `deltaTime()`, `beginFrame()`... (vía puntero/referencia, no Vulkan) |
| `ctx.id()` | Su `PluginID` |
| `ctx.events()` | El `EventBus` **compartido** del motor (`engine.eventBus()`) |
| `ctx.services()` | El `ServiceRegistry` **compartido** del motor (`engine.serviceRegistry()`) |
| `ctx.api()` | Su `ApiPlugin` (logs y tiempo) |

> **Nota:** bus y registro de servicios **son únicos por motor**: dos plugins
> ven los mismos (p. ej. Physics puede `provide()` un servicio que otro
> `consume()`; un evento publicado por uno lo recibe el otro). El `PluginContext`
> sigue siendo propiedad del plugin (solo el puntero de arranque es suyo).

### EventBus

```cpp
using namespace Uron::Plugin;

uint32_t h = ctx.events().subscribe("player-died", [](const Event& e) {
    // e.data es std::any: std::any_cast<int>(e.data)
});

ctx.events().publish({"player-died", 42});
ctx.events().unsubscribe("player-died", h);
```

`Event { const char* type; std::any data; }` — simple, por tipo de cadena.

### ServiceRegistry

```cpp
struct PhysicsService {
    std::function<Vec3(BodyID)> getPosition;
};

// quien ofrece
ctx.services().provide<PhysicsService>("physics", &m_service);

// quien consume (nullptr si no existe o el tipo no coincide)
auto* svc = ctx.services().consume<PhysicsService>("physics");
```

Comprueba `has("physics")` antes si el servicio es opcional.

### PluginConfig (key-value)

```cpp
Uron::Plugin::PluginConfig cfg;
cfg.set("gravity", "-9.8");
float g = cfg.getFloat("gravity");     // getFloat/getInt/getBool/get/has/remove
```

Útil para cargar la configuración de un plugin desde JSON/ini por tu cuenta.

### IPluginApi<T>

```cpp
class Api { /* ... */ };
class Plugin : public IPlugin, public IPluginApi<Api> {
    Api* api() override { return &m_api; }
};
// usuario: engine.plugin<Plugin>().api()->algo();
```

Interfaz opcional para exponer una sub-API tipada dentro del plugin.

### ApiPlugin (`ctx.api()`)

| Método | Estado |
|---|---|
| `logInfo/logWarn/logError(msg)` | ✅ funciona (usa el Logger del motor) |
| `deltaTime()`, `totalTime()` | ✅ funciona |
| `services()` | ✅ el `ServiceRegistry` compartido del motor |
| `onUpdate/onRender/onEvent(cb)` | ⚠️ guardan el callback; el bucle actual **no los invoca** aún (usa `onUpdate` de `IPlugin`) |
| `renderer()/scene2D()/scene3D()` | ⚠️ stub: devuelven `nullptr` |
| `loadTexture()/loadShader()` | ⚠️ stub: devuelven `nullptr` |

---

## 6. Reglas duras (no romper)

- ❌ Incluir `<vulkan/vulkan.h>` o usar `Vk*` en el plugin.
- ❌ Estado global (`static` con mutable compartido).
- ❌ Exponer GLFW o STL exótica en la cabecera pública del plugin.
- ❌ Meter lógica de juego en el core (eso es de plugins).
- ✅ Logs con `URON_INFO/WARN/ERROR` o `ctx.api().log*`.
- ✅ Compilar sin errores ni warnings (`-Wall -Wextra -Wpedantic`).

## 7. Checklist antes de dar por bueno un plugin

- [ ] Namespace `Uron::Plugin::<Nombre>` + hereda `IPlugin`.
- [ ] `ID/NAME/VERSION/AUTHOR` constexpr.
- [ ] `onLoad` devuelve `false` si algo falla (el motor lo deshace).
- [ ] Registrado con `URON_PLUGIN(NS, CLASS)`.
- [ ] `plugins/CMakeLists.txt` con `add_subdirectory(...)`.
- [ ] CMake con alias `Uron::Plugin::<Nombre>` + link a `Uron::Core`.
- [ ] Ejemplo en `examples/` que lo importe y use.
- [ ] Añadido a `map_project.txt` (mismo commit).

Siguiente: [04 — Arquitectura y estado](04-arquitectura-y-estado.md).
