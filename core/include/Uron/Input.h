// ============================================================================
//  Input.h
//  ---------------------------------------------------------------------------
//  QUE ES: Estado de teclado y raton del frame actual.
//  CONTIENE: enum class Key (teclas), enum class MouseButton (botones del
//            raton) y la clase Input con consultas por frame:
//            isKeyDown/isKeyPressed/isKeyReleased, mousePosition, mouseDelta,
//            scrollDelta.
//  PARA QUE: Que el juego pregunte "¿esta pulsada la W?" sin ver GLFW ni
//            registrar callbacks a mano. Las consultas son estables durante
//            el frame: los bordes (pressed/released) duran exactamente un
//            frame, aunque se pierda un tap rapidisimo.
//  QUIEN LO USA: El usuario via engine.input(). El motor alimenta los
//                eventos desde la Window en Engine::attachWindow(); nadie
//                debe llamar a handle*/beginFrame() desde codigo de juego.
//  EJEMPLO:
//     const auto& in = engine.input();
//     if (in.isKeyDown(Uron::Key::Escape)) window.requestClose();
//     if (in.isKeyPressed(Uron::Key::Space)) saltar();
//     sprite->translate(in.mouseDelta());
// ============================================================================
#pragma once
#include <Uron/Types.h>
#include <Uron/math/Vec2.h>

namespace Uron {

// Codigos propios, independientes del backend de ventana (la traduccion a
// GLFW vive en Input.cpp). Los rangos A-Z, Num0-9, F1-F12 y KP0-KP9 son
// contiguos (ver static_assert en Input.cpp).
enum class Key : i32 {
    Unknown = -1,

    Space = 0,
    Enter, Tab, Backspace, Escape, Delete, Insert,
    Right, Left, Down, Up, PageUp, PageDown, Home, End,

    CapsLock, ScrollLock, NumLock, PrintScreen, Pause, Menu,

    LeftShift, LeftControl, LeftAlt, LeftSuper,
    RightShift, RightControl, RightAlt, RightSuper,

    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    Minus, Equal, LeftBracket, RightBracket, Backslash,
    Semicolon, Apostrophe, Grave, Comma, Period, Slash,

    KP0, KP1, KP2, KP3, KP4, KP5, KP6, KP7, KP8, KP9,
    KPDecimal, KPDivide, KPMultiply, KPSubtract, KPAdd, KPEnter, KPEqual
};

enum class MouseButton : i32 {
    Left   = 0,
    Right  = 1,
    Middle = 2,
    X1     = 3,
    X2     = 4
};

class Input {
public:
    Input();
    ~Input();

    Input(const Input&)            = delete;
    Input& operator=(const Input&) = delete;

    // ==================== Teclado ====================
    bool isKeyDown(Key k) const;      // mantenido ahora
    bool isKeyPressed(Key k) const;   // borde: empezo este frame
    bool isKeyReleased(Key k) const;  // borde: solto este frame

    // ==================== Raton ====================
    bool isMouseDown(MouseButton b) const;
    bool isMousePressed(MouseButton b) const;
    bool isMouseReleased(MouseButton b) const;

    Vec2 mousePosition() const;  // en pixeles, respecto a la ventana
    Vec2 mouseDelta() const;     // movimiento acumulado del frame
    Vec2 scrollDelta() const;    // rueda del frame (x, y)

    // ==================== Uso interno del motor ====================
    //  Alimentados por las callbacks de la Window. NO llamar desde el
    //  codigo de juego: las consultas de arriba son la API publica.
    void handleKey(i32 key, i32 action);
    void handleMouseButton(i32 button, i32 action);
    void handleMouseMove(f32 x, f32 y);
    void handleScroll(f32 dx, f32 dy);
    void beginFrame();  // promueve los bordes pendientes al frame actual
    void reset();       // borra todo el estado (attach/detach de ventana)

private:
    struct Impl;
    Impl* impl;
};

}
