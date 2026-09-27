// ============================================================================
//  Window.h
//  ---------------------------------------------------------------------------
//  QUE ES: La ventana del sistema operativo.
//  CONTIENE: WindowConfig (titulo, tamano, flags), WindowCallbacks (input),
//            y la clase Window con create(), destroy(), pollEvents(),
//            shouldClose().
//  PARA QUE: Que el usuario cree SU ventana. El motor no la crea por el.
//  QUIEN LO USA: El usuario (crea la ventana), el motor (la referencia
//                para renderizar).
//  EJEMPLO:
//     Uron::Window window;
//     window.create({.title="Mi Juego", .width=1280, .height=720});
// ============================================================================
#pragma once
#include <Uron/Types.h>
#include <functional>
#include <string>
#include <string_view>

namespace Uron {

struct WindowConfig {
    std::string title      = "Uron";
    u32         width      = 1280;
    u32         height     = 720;
    bool        resizable  = true;
    bool        fullscreen = false;
    bool        vsync      = true;
    i32         samples    = 1;
};

struct WindowCallbacks {
    std::function<void(i32 key, i32 action)> onKey;
    std::function<void(f32 x, f32 y)>        onMouseMove;
    std::function<void(u32 w, u32 h)>        onResize;
    std::function<void()>                    onClose;
};

class Window {
public:
    Window();
    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) noexcept;
    Window& operator=(Window&&) noexcept;

    bool create(const WindowConfig& cfg);
    void destroy();

    bool shouldClose() const;
    void pollEvents();
    void requestClose();

    void setTitle(std::string_view title);
    void setSize(u32 w, u32 h);
    void setFullscreen(bool enable);
    void setVSync(bool enable);

    std::string title()  const;
    u32         width()  const;
    u32         height() const;
    bool        vsync()  const;

    void setCallbacks(const WindowCallbacks& cbs);

    void* nativeHandle() const;
    void* nativeDisplay() const;
    bool  isValid() const;

private:
    struct Impl;
    Impl* impl;
};

}