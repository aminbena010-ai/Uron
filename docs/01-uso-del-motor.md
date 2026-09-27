# 01 — Uso del motor

Guía práctica para arrancar un proyecto con Uron: compilar el motor, crear la
ventana, correr el bucle, leer el input y registrar mensajes.

---

## 1. Requisitos y build

| Requisito | Versión |
|---|---|
| Compilador | C++17 (GCC/Clang/MSVC) |
| CMake | 3.20+ |
| Vulkan SDK | 1.2+ (`find_package(Vulkan)`) |
| GLFW | 3.3+ (`find_package(glfw3)`) |
| glslc | opcional — compila los shaders `.vert/.frag` a `.spv` |

```bash
cmake -S . -B build          # configura
cmake --build build -j       # compila (0 errores, 0 warnings esperados)
```

- **Binarios:** `build/bin/` (ejemplos) · **Shaders:** `build/bin/shaders/`
  · **Assets:** `build/bin/assets/`. Los ejemplos se ejecutan desde `build/bin/`
  porque las rutas de assets y shaders son relativas al CWD.
- **Opciones CMake:**

  | Opción | Por defecto | Qué hace |
  |---|---|---|
  | `URON_BUILD_EXAMPLES` | `ON` | Compila `examples/hello_*` |
  | `URON_BUILD_PLUGINS`  | `ON` | Compila los plugins oficiales (`plugins/`) |
  | `URON_BUILD_TESTS`    | `OFF` | Reservado para tests (aún sin targets) |

  ```bash
  cmake -S . -B build -DURON_BUILD_EXAMPLES=OFF   # solo la librería
  ```

- Los `.spv` **no** están en git: los genera CMake con `glslc` en cada build
  (`uron_shaders`). Si no hay `glslc`, usa `shaders/compile.sh` a mano.

---

## 2. Ciclo de vida del motor

```
Engine::init()                     crea PluginManager, marca running
Window::create(cfg)                crea la ventana (GLFW)
Engine::attachWindow(&window)      crea VulkanRenderer + cablea Input
   ├── engine.setScene(&scene)     opcional: activa escena (onEnter)
   └── engine.importPlugin<T>()    opcional: carga plugins (onLoad)

   while (!window.shouldClose()) {
       window.pollEvents();        // eventos del SO → callbacks → Input
       engine.beginFrame();        // delta, input.beginFrame(), update de
                                   //   escena y plugins, renderer.beginFrame
       engine.clear(color);        // color de fondo
       engine.endFrame();          // render de escena + presentación
   }

   engine.setScene(nullptr);       // opcional: onExit de la escena
   engine.shutdown();              // onUnload de plugins, destruye Vulkan
   window.destroy();               // destruye la ventana
```

Puntos importantes:

- `attachWindow()` **inicializa Vulkan**: sin GPU/entorno gráfico falla y
  devuelve `false`. `Engine::init()` solo funciona "sin ventana, sin Vulkan"
  (útil para pruebas de lógica).
- El orden del bucle importa: `pollEvents()` **antes** de `beginFrame()`, para
  que el input del frame vea los eventos recientes.
- `beginFrame()` calcula `deltaTime()`/`totalTime()`, promueve los bordes del
  input, actualiza la escena (`update`) y los plugins (`onUpdate`), y arranca
  el frame de render.
- `Engine` es pimpl: sus punteros internos no aparecen en la API pública.

---

## 3. Window

Cabecera: `core/include/Uron/Window.h`. GLFW está **oculto**: el usuario solo
ve `Uron::Window`.

```cpp
Uron::Window window;
Uron::WindowConfig cfg;
cfg.title      = "Mi Juego";
cfg.width      = 1280;
cfg.height     = 720;
cfg.resizable  = true;
cfg.fullscreen = false;
cfg.vsync      = true;
cfg.samples    = 1;

if (!window.create(cfg)) return -1;   // GLFW NO_API: Vulkan dibuja
```

| Método | Uso |
|---|---|
| `pollEvents()` | Bombea los eventos del sistema operativo (llamar 1× por frame) |
| `shouldClose()` | `true` si el usuario pidió cerrar (o `requestClose()`) |
| `requestClose()` | Cierra programáticamente (p. ej. con `Key::Escape`) |
| `setTitle()` / `setSize()` / `setFullscreen()` / `setVSync()` | Propiedades |
| `width()` / `height()` | Tamaño del **framebuffer** (≠ tamaño lógico en HiDPI) |
| `nativeHandle()` | `void*` al handle interno — **no usar** en código de juego |

### Callbacks del usuario

```cpp
Uron::WindowCallbacks cbs;
cbs.onKey         = [](i32 key, i32 action)    { /* ... */ };
cbs.onMouseButton = [](i32 button, i32 action) { /* ... */ };
cbs.onMouseMove   = [](f32 x, f32 y)           { /* ... */ };
cbs.onScroll      = [](f32 dx, f32 dy)         { /* ... */ };
cbs.onResize      = [](u32 w, u32 h)           { /* ... */ };
cbs.onClose       = []()                       { /* ... */ };
window.setCallbacks(cbs);
```

Los callbacks del usuario viven en **un canal propio**: no pisan los que el
motor usa para alimentar `Input`, así que pueden coexistir sin problemas.

---

## 4. Input (`engine.input()`)

Cabecera: `core/include/Uron/Input.h`. Teclado y ratón, cableados por el
motor en `attachWindow()`.

```cpp
const auto& in = engine.input();

// Teclado
if (in.isKeyDown(Uron::Key::W))        moverArriba();     // mantenido
if (in.isKeyPressed(Uron::Key::Space)) saltar();          // borde: 1 frame
if (in.isKeyReleased(Uron::Key::Space)) soltar();         // borde: 1 frame

// Raton
if (in.isMouseDown(Uron::MouseButton::Left))  arrastrar();
Uron::Vec2 pos   = in.mousePosition();   // pixeles en la ventana
Uron::Vec2 delta = in.mouseDelta();      // movimiento del frame
Uron::Vec2 roll  = in.scrollDelta();     // rueda del frame (x, y)

if (in.isKeyPressed(Uron::Key::Escape)) window.requestClose();
```

Semántica:

- **`isKeyDown`** = estado actual; **`isKeyPressed/Released`** = bordes que
  duran **exactamente un frame**. Un tap más rápido que un frame no se pierde:
  los eventos se acumulan en "pendientes" y se promueven en `beginFrame()`.
- `mouseDelta()` es el movimiento acumulado desde el `beginFrame()` anterior.
  El primer movimiento de ratón solo fija la posición (no genera un delta
  falso desde `(0,0)`).
- **Sin ventana adjunta**, todo consulta `false`.
- Las claves `handle*()`/`beginFrame()`/`reset()` del header son **uso interno
  del motor**: el código de juego solo debe consultar.

Teclas disponibles: `Key::A..Z`, `Num0..Num9`, `F1..F12`, flechas, modificador
(`LeftShift`, `LeftControl`, `LeftAlt`, `LeftSuper` + lado derecho), espacio,
enter, tab, escape, navegación (`Home`, `End`, `PageUp`...), keypad
(`KP0..KP9`, `KPAdd`...) y puntuación (`Comma`, `Period`, `Slash`...).
Botones: `Left`, `Right`, `Middle`, `X1`, `X2`.

Ejemplo completo: `examples/hello_input/`.

---

## 5. Logger

```cpp
Uron::Logger::setLevel(Uron::LogLevel::Info);   // Trace|Info|Warn|Error|Fatal

URON_TRACE("detalle");
URON_INFO("frame actualizado");   // se expande a Logger::info(...)
URON_WARN("cache miss");
URON_ERROR("fallo al cargar assets/player.png");
URON_FATAL("sin GPU");

// No hay formato printf: construye el string antes.
char buf[64];
std::snprintf(buf, sizeof(buf), "pos = (%.1f, %.1f)", p.x, p.y);
URON_INFO(buf);
```

- Salida con color y timestamp por nivel; protegido con mutex.
- Las macros aceptan un `std::string_view` (literal o `std::string`); para
  formatos usa `std::snprintf` como en los ejemplos.

---

## 6. Tipos y matemáticas

- **Alias** (`Uron/Types.h`): `u8..u64`, `i8..i64`, `f32`, `f64`, `struct Rect`.
- **Vectores:** `Vec2`, `Vec3`, `Vec4` — operadores, `length`, `dot`, `cross`,
  `normalized()`.
- **Matrices:** `Mat3`, `Mat4` — `mul`, `translate/rotate/scale`,
  `perspective/ortho`, `inverse`, `transpose` (notación: `a * b` aplica `b`
  primero, matrices columna).
- **Cuaterniones:** `Quat` — `fromAxisAngle`, `fromEuler`, `slerp`, `toMat4`.
- **Utilidades** (`Math.h`): `PI/TAU`, `radians/degrees`, `clamp`, `lerp`,
  `min/max`, `EPSILON`.
- **Color** (`render/Color.h`): `Color{r,g,b,a}` + fábricas `Color::Black()`,
  `White()`, `Red()`, `fromHex(...)`.

---

## 7. Ejemplos del repositorio

| Ejemplo | Qué demuestra |
|---|---|
| `hello_uron` | Ventana + callbacks + shader custom (WIP) |
| `hello_scene` | Árbol de escena, clases `Node2D` propias, `onUpdate` |
| `hello_sprite` | Sprite con textura `assets/player.png` |
| `hello_shader` | Shader custom con `setUniform` por frame |
| `hello_input` | Teclado/ratón con `engine.input()` |
| `hello_physics` | Plugin Physics: `importPlugin`, `createBody`, `step` |

```bash
cd build/bin && ./hello_input
```

Siguiente: [02 — Escena y render](02-escena-y-render.md).
