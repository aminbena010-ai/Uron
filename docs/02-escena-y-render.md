# 02 — Escena y render

Cómo organizar el contenido (árbol de escena 2D) y cómo lo dibuja el motor:
sprites, texturas, la interfaz `Renderer` y los shaders custom.

---

## 1. Scene2D y el árbol de nodos

`Scene2D` (`core/include/Uron/scene/Scene2D.h`) posee un nodo raíz y recorre
el árbol en cada frame.

```cpp
Uron::Scene2D scene("MainScene");
scene.setClearColor(Uron::Color::Black());

auto sprite = std::make_unique<Uron::Sprite2D>("Player");
sprite->setPosition({320.f, 180.f});
scene.root()->addChild(std::move(sprite));   // devuelve Node2D* (no propietario)

engine.setScene(&scene);   // llama a enter() sobre todo el árbol
```

Ciclo (lo ejecuta `Engine`):

| Momento | Qué pasa |
|---|---|
| `engine.setScene(&scene)` | `onEnter(scene)` en cada nodo |
| `engine.beginFrame()` | `scene.update(dt)` → `onUpdate(scene, dt)` en todo el árbol |
| `engine.endFrame()` | `scene.render(renderer)` → `onRender(renderer)` en todo el árbol |
| `engine.setScene(nullptr)` | `onExit(scene)` en cada nodo |

### Node2D

Clase base de todos los nodos (`scene/Node2D.h`):

- **Identidad:** `name()`, `setName()`.
- **Transform:** `position()`, `rotation()` (radianes), `scale()`;
  `setPosition/rotate/translate/scale`.
- **Jerarquía:** `addChild(unique_ptr)` → `Node2D*` crudo (la propiedad es del
  padre); `removeChild(Node2D*)` → `unique_ptr` (la propiedad vuelve a ti);
  `children()`, `parent()`.
- **Visibilidad:** `isVisible()/setVisible()`; **tinte:** `tint()/setTint()`
  (se guarda, pero el sprite pipeline aún no lo aplica).
- **Ciclo virtual:** `onEnter`, `onUpdate`, `onExit`, `onRender`.

Clase propia de juego (patrón de `examples/hello_scene`):

```cpp
class Player : public Uron::Node2D {
public:
    Player() : Uron::Node2D("Player") {}

    void onUpdate(Uron::Scene2D& scene, float dt) override {
        (void)scene;
        m_time += dt;
        setPosition({640.f + 300.f * std::sin(m_time), 360.f});
    }

private:
    float m_time = 0.f;
};
```

> Los nodos no tienen acceso directo a `Engine`; consulta el input desde el
> bucle principal (como hace `hello_input`) o pásale un `const Uron::Input*`
> al nodo.

---

## 2. Sprite2D

`Sprite2D : Node2D` — un rectángulo con textura y shader opcional:

```cpp
auto s = std::make_unique<Uron::Sprite2D>("Player");
s->setTexture("assets/player.png");   // ruta relativa al CWD (build/bin)
s->setSize({256.f, 256.f});           // tamano en pixeles
s->setPosition({512.f, 360.f});
s->setShader(&miShader);              // opcional (ver seccion 5)
```

- En `onRender` compone la `Mat4` global del nodo (**jerarquía padre→hijo,
  con rotación/escala/tinte heredados**) y llama a
  `renderer.drawSprite(texture, matrix, shader, tint)`.
- **Coordenadas:** sistema de pixeles con origen arriba-izquierda; el pipeline
  por defecto convierte con `screenSize` (tamaño del framebuffer).
- `rotation()`/`scale()`/`tint()` del `Node2D` **sí se aplican** (se componen
  en `world`, acumulando la del padre); `setPosition` tras `addChild` funciona
  porque la matriz se recalcula en cada `render()`.
- `setVisible(false)` oculta el nodo **y todo su subárbol** (update y render
  se saltan la rama).

---

## 3. Camera2D

```cpp
auto cam = std::make_unique<Uron::Camera2D>("MainCamera");
cam->setViewportSize(1280.f, 720.f);
cam->setZoom(1.5f);
cam->setPosition({100.f, 50.f});
scene.root()->addChild(std::move(cam));
```

Expone `viewMatrix()` (ortográfica centrada + zoom + rotación + traslación)
en **espacio de píxeles** (origen arriba-izquierda): `T(center)·S(zoom)·R(−rot)·T(−pos)`.

> **Estado:** `Scene2D::setCamera(&cam)` (no-owning, no la destruyas antes
> que la escena) la consume en `render()`; sin cámara la vista es identidad
> (se dibuja en coordenadas de pantalla).

---

## 4. Renderer (interfaz)

`render/Renderer.h` es la **interfaz abstracta** que implementa
`VulkanRenderer` (privado en `core/src/vulkan/`). El usuario la ve como
referencia (p. ej. en `onRender`):

```cpp
void onRender(Uron::Renderer& renderer) override {
    renderer.drawSprite(m_texture, transform, m_shader);
}
```

| Método | Uso |
|---|---|
| `beginFrame/clear/endFrame` | Los llama `Engine` (no llamar a mano) |
| `drawSprite(tex, mat, shader*, tint)` | Dibuja un sprite 2D (`tint` opcional, `Color::White()` por defecto) |
| `drawMesh(mesh, transform)` | Dibuja una malla (`MeshBuilder::triangle/quad/cube/circle`) |
| `setViewProjection(vp)` | Matriz vista-proyección para `drawMesh` (por defecto identidad) |
| `setViewport` / `setClearColor` | Estado del frame |
| `width()/height()` | Tamaño del framebuffer |
| `waitIdle()` | Espera a la GPU (cuidado: bloquea) |

Nunca aparecen tipos de Vulkan en esta cabecera (regla del proyecto).

---

## 5. Texturas

```cpp
Uron::Texture tex;
tex.loadFromFile("assets/player.png");          // PNG/JPG vía stb_image

Uron::TextureDesc desc;
desc.filter  = Uron::TextureFilter::Nearest;    // pixel-art
desc.wrap    = Uron::TextureWrap::Repeat;
desc.mipmaps = false;
tex.loadFromFile("assets/atlas.png", desc);

const Uron::u8 px[4] = {255, 255, 255, 255};    // 1x1 blanco
tex.loadFromMemory(px, 1, 1);
```

- `isValid()`, `width()`, `height()`, `handle()`.
- Cada textura válida consume un descriptor set en el renderer (cacheado por
  textura). El único `#define STB_IMAGE_IMPLEMENTATION` está en
  `core/src/render/Texture.cpp` — **no repetirlo**.

---

## 6. Shaders custom

### Compilación

Uron **no compila GLSL en runtime** (no existe `loadFromSource()`; la API
solo carga `.spv` con `loadFromFile`). Los `.spv` los genera CMake:

```
shaders/wave.vert ──glslc──▶ build/bin/shaders/wave.vert.spv
shaders/wave.frag ──glslc──▶ build/bin/shaders/wave.frag.spv
```

Basta con dejar `.vert`/`.frag` en `shaders/` y recompilar (o
`shaders/compile.sh` a mano).

### Uso

```cpp
Uron::Shader wave;
if (!wave.loadFromFile("shaders/wave.vert.spv",
                       "shaders/wave.frag.spv")) {
    return -1;   // falta el .spv o ruta incorrecta (relativa a build/bin)
}

sprite->setShader(&wave);

while (running) {
    // El ORDEN de setUniform importa (ver abajo).
    wave.setUniform("time", t);
    wave.setUniform("amplitude", 8.f);
    wave.setUniform("tint", Uron::Color{1.f, 0.85f, 0.4f, 1.f});
    // ...
}
```

API: `setUniform(name, f32 | i32 | Vec2 | Vec3 | Vec4 | Color | Mat4)`,
`hasUniform(name)`, `isValid()`, `bind()/unbind()` (uso interno),
`ShaderDesc{depthTest, depthWrite, blending, wireframe}` como tercer
parámetro de `loadFromFile`.

### Regla de push constants (importante)

El pipeline custom empuja **128 bytes** en el push constant:

```
offset  0: vec4 row0       (a b c d — matriz afín 2D)
offset 16: vec4 row1       (e f screenW screenH)
offset 32: vec4 spriteTint
offset 48: uniforms del usuario  (alineacion std430, max 80 bytes)
```

- Los uniforms se empaquetan **en el orden de la PRIMERA `setUniform()` de
  cada nombre**, que debe coincidir con el orden de declaración del bloque
  `layout(push_constant)` del GLSL.
- Los no fijados viajan como `0`.
- El pipeline de sprites por defecto usa solo los 48 bytes del sprite
  (`sprite.vert`/`sprite.frag`).

Ver `shaders/wave.vert` y `examples/hello_shader/` como referencia.

---

## 7. Estado conocido del render

- Sprites 2D con pipeline por defecto y con shaders custom: **conectados**.
- Si la creación del pipeline custom falla, el motor avisa una vez y usa el
  sprite pipeline por defecto.
- Sincronización: 2 frames en flight (`MAX_FRAMES = 2`); `renderFinished`
  es **por imagen de swapchain**.
- Validación Vulkan (`VK_LAYER_KHRONOS_validation` + debug messenger, activa
  en Debug o con `URON_VALIDATION=1`): los 7 ejemplos corren sin mensajes.
- 3D: `drawMesh` + pipeline propio (depth D32, MVP por push, viewport
  dinámico) + ejemplo `hello_cube` **conectados**; quedan el grafo de
  escenas 3D y más iluminación (el frag aplica lambert direccional).

Siguiente: [03 — Plugins](03-plugins.md).
