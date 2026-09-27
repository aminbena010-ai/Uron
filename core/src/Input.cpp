#include <Uron/Input.h>
#include <GLFW/glfw3.h>
#include <algorithm>

namespace Uron {

namespace {

constexpr i32 KeyCount   = 512;
constexpr i32 MouseCount = 8;

// Los rangos contiguos del enum se traducen por aritmetica; el resto, uno
// a uno. Devuelve -1 (Key::Unknown o fuera de rango) si no hay codigo GLFW.
i32 toGlfw(Key k) {
    const i32 v = static_cast<i32>(k);

    if (k >= Key::A && k <= Key::Z)
        return GLFW_KEY_A + (v - static_cast<i32>(Key::A));
    if (k >= Key::Num0 && k <= Key::Num9)
        return GLFW_KEY_0 + (v - static_cast<i32>(Key::Num0));
    if (k >= Key::F1 && k <= Key::F12)
        return GLFW_KEY_F1 + (v - static_cast<i32>(Key::F1));
    if (k >= Key::KP0 && k <= Key::KP9)
        return GLFW_KEY_KP_0 + (v - static_cast<i32>(Key::KP0));

    switch (k) {
        case Key::Space:       return GLFW_KEY_SPACE;
        case Key::Enter:       return GLFW_KEY_ENTER;
        case Key::Tab:         return GLFW_KEY_TAB;
        case Key::Backspace:   return GLFW_KEY_BACKSPACE;
        case Key::Escape:      return GLFW_KEY_ESCAPE;
        case Key::Delete:      return GLFW_KEY_DELETE;
        case Key::Insert:      return GLFW_KEY_INSERT;
        case Key::Right:       return GLFW_KEY_RIGHT;
        case Key::Left:        return GLFW_KEY_LEFT;
        case Key::Down:        return GLFW_KEY_DOWN;
        case Key::Up:          return GLFW_KEY_UP;
        case Key::PageUp:      return GLFW_KEY_PAGE_UP;
        case Key::PageDown:    return GLFW_KEY_PAGE_DOWN;
        case Key::Home:        return GLFW_KEY_HOME;
        case Key::End:         return GLFW_KEY_END;
        case Key::CapsLock:    return GLFW_KEY_CAPS_LOCK;
        case Key::ScrollLock:  return GLFW_KEY_SCROLL_LOCK;
        case Key::NumLock:     return GLFW_KEY_NUM_LOCK;
        case Key::PrintScreen: return GLFW_KEY_PRINT_SCREEN;
        case Key::Pause:       return GLFW_KEY_PAUSE;
        case Key::Menu:        return GLFW_KEY_MENU;
        case Key::LeftShift:      return GLFW_KEY_LEFT_SHIFT;
        case Key::LeftControl:    return GLFW_KEY_LEFT_CONTROL;
        case Key::LeftAlt:        return GLFW_KEY_LEFT_ALT;
        case Key::LeftSuper:      return GLFW_KEY_LEFT_SUPER;
        case Key::RightShift:     return GLFW_KEY_RIGHT_SHIFT;
        case Key::RightControl:   return GLFW_KEY_RIGHT_CONTROL;
        case Key::RightAlt:       return GLFW_KEY_RIGHT_ALT;
        case Key::RightSuper:     return GLFW_KEY_RIGHT_SUPER;
        case Key::Minus:       return GLFW_KEY_MINUS;
        case Key::Equal:       return GLFW_KEY_EQUAL;
        case Key::LeftBracket:   return GLFW_KEY_LEFT_BRACKET;
        case Key::RightBracket:  return GLFW_KEY_RIGHT_BRACKET;
        case Key::Backslash:   return GLFW_KEY_BACKSLASH;
        case Key::Semicolon:   return GLFW_KEY_SEMICOLON;
        case Key::Apostrophe:  return GLFW_KEY_APOSTROPHE;
        case Key::Grave:       return GLFW_KEY_GRAVE_ACCENT;
        case Key::Comma:       return GLFW_KEY_COMMA;
        case Key::Period:      return GLFW_KEY_PERIOD;
        case Key::Slash:       return GLFW_KEY_SLASH;
        case Key::KPDecimal:  return GLFW_KEY_KP_DECIMAL;
        case Key::KPDivide:    return GLFW_KEY_KP_DIVIDE;
        case Key::KPMultiply:  return GLFW_KEY_KP_MULTIPLY;
        case Key::KPSubtract:  return GLFW_KEY_KP_SUBTRACT;
        case Key::KPAdd:       return GLFW_KEY_KP_ADD;
        case Key::KPEnter:     return GLFW_KEY_KP_ENTER;
        case Key::KPEqual:     return GLFW_KEY_KP_EQUAL;
        default:               return -1;
    }
}

static_assert(static_cast<i32>(Key::Z)  - static_cast<i32>(Key::A)   == 25,
              "El rango A-Z del enum Key debe ser contiguo");
static_assert(static_cast<i32>(Key::Num9) - static_cast<i32>(Key::Num0) == 9,
              "El rango Num0-Num9 del enum Key debe ser contiguo");
static_assert(static_cast<i32>(Key::F12) - static_cast<i32>(Key::F1) == 11,
              "El rango F1-F12 del enum Key debe ser contiguo");
static_assert(static_cast<i32>(Key::KP9) - static_cast<i32>(Key::KP0) == 9,
              "El rango KP0-KP9 del enum Key debe ser contiguo");
static_assert(static_cast<i32>(MouseButton::Left)   == GLFW_MOUSE_BUTTON_LEFT,
              "MouseButton::Left debe coincidir con GLFW");
static_assert(static_cast<i32>(MouseButton::Right)  == GLFW_MOUSE_BUTTON_RIGHT,
              "MouseButton::Right debe coincidir con GLFW");
static_assert(static_cast<i32>(MouseButton::Middle) == GLFW_MOUSE_BUTTON_MIDDLE,
              "MouseButton::Middle debe coincidir con GLFW");

}

struct Input::Impl {
    // down se actualiza con cada evento; pressed/released son los bordes
    // visibles durante el frame y pending acumula los eventos llegados
    // desde el beginFrame() anterior (se promueven en el siguiente).
    bool down[KeyCount]{};
    bool pressed[KeyCount]{};
    bool released[KeyCount]{};
    bool pendingPressed[KeyCount]{};
    bool pendingReleased[KeyCount]{};

    bool mouseDown[MouseCount]{};
    bool mousePressed[MouseCount]{};
    bool mouseReleased[MouseCount]{};
    bool pendingMousePressed[MouseCount]{};
    bool pendingMouseReleased[MouseCount]{};

    Vec2 position{};
    Vec2 deltaPending{};
    Vec2 deltaFrame{};
    Vec2 scrollPending{};
    Vec2 scrollFrame{};
    bool hasMouse = false;
};

Input::Input() : impl(new Impl) {}

Input::~Input() {
    delete impl;
}

bool Input::isKeyDown(Key k) const {
    const i32 c = toGlfw(k);
    return c >= 0 && impl->down[c];
}

bool Input::isKeyPressed(Key k) const {
    const i32 c = toGlfw(k);
    return c >= 0 && impl->pressed[c];
}

bool Input::isKeyReleased(Key k) const {
    const i32 c = toGlfw(k);
    return c >= 0 && impl->released[c];
}

bool Input::isMouseDown(MouseButton b) const {
    const i32 c = static_cast<i32>(b);
    return c >= 0 && c < MouseCount && impl->mouseDown[c];
}

bool Input::isMousePressed(MouseButton b) const {
    const i32 c = static_cast<i32>(b);
    return c >= 0 && c < MouseCount && impl->mousePressed[c];
}

bool Input::isMouseReleased(MouseButton b) const {
    const i32 c = static_cast<i32>(b);
    return c >= 0 && c < MouseCount && impl->mouseReleased[c];
}

Vec2 Input::mousePosition() const {
    return impl->position;
}

Vec2 Input::mouseDelta() const {
    return impl->deltaFrame;
}

Vec2 Input::scrollDelta() const {
    return impl->scrollFrame;
}

void Input::handleKey(i32 key, i32 action) {
    if (key < 0 || key >= KeyCount) return;

    if (action == GLFW_PRESS) {
        impl->down[key] = true;
        impl->pendingPressed[key] = true;
    } else if (action == GLFW_RELEASE) {
        impl->down[key] = false;
        impl->pendingReleased[key] = true;
    }
    // GLFW_REPEAT: el estado down ya es true, no hay nada que actualizar.
}

void Input::handleMouseButton(i32 button, i32 action) {
    if (button < 0 || button >= MouseCount) return;

    if (action == GLFW_PRESS) {
        impl->mouseDown[button] = true;
        impl->pendingMousePressed[button] = true;
    } else if (action == GLFW_RELEASE) {
        impl->mouseDown[button] = false;
        impl->pendingMouseReleased[button] = true;
    }
}

void Input::handleMouseMove(f32 x, f32 y) {
    const Vec2 p{x, y};
    // El primer evento solo fija la posicion: si no, el delta inicial
    // seria el salto desde (0,0) y un sprite lo "explotaria".
    if (impl->hasMouse) {
        impl->deltaPending += p - impl->position;
    } else {
        impl->hasMouse = true;
    }
    impl->position = p;
}

void Input::handleScroll(f32 dx, f32 dy) {
    impl->scrollPending += Vec2{dx, dy};
}

void Input::beginFrame() {
    std::copy_n(impl->pendingPressed, KeyCount, impl->pressed);
    std::copy_n(impl->pendingReleased, KeyCount, impl->released);
    std::fill_n(impl->pendingPressed, KeyCount, false);
    std::fill_n(impl->pendingReleased, KeyCount, false);

    std::copy_n(impl->pendingMousePressed, MouseCount, impl->mousePressed);
    std::copy_n(impl->pendingMouseReleased, MouseCount, impl->mouseReleased);
    std::fill_n(impl->pendingMousePressed, MouseCount, false);
    std::fill_n(impl->pendingMouseReleased, MouseCount, false);

    impl->deltaFrame  = impl->deltaPending;
    impl->deltaPending = Vec2{};
    impl->scrollFrame  = impl->scrollPending;
    impl->scrollPending = Vec2{};
}

void Input::reset() {
    std::fill_n(impl->down, KeyCount, false);
    std::fill_n(impl->pressed, KeyCount, false);
    std::fill_n(impl->released, KeyCount, false);
    std::fill_n(impl->pendingPressed, KeyCount, false);
    std::fill_n(impl->pendingReleased, KeyCount, false);

    std::fill_n(impl->mouseDown, MouseCount, false);
    std::fill_n(impl->mousePressed, MouseCount, false);
    std::fill_n(impl->mouseReleased, MouseCount, false);
    std::fill_n(impl->pendingMousePressed, MouseCount, false);
    std::fill_n(impl->pendingMouseReleased, MouseCount, false);

    impl->position     = Vec2{};
    impl->deltaPending = Vec2{};
    impl->deltaFrame   = Vec2{};
    impl->scrollPending = Vec2{};
    impl->scrollFrame   = Vec2{};
    impl->hasMouse = false;
}

}
