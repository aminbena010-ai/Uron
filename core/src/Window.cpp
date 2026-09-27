#include <Uron/Window.h>
#include <Uron/Logger.h>
#include <GLFW/glfw3.h>
#include <string>

namespace Uron {

struct Window::Impl {
    GLFWwindow*     handle   = nullptr;
    WindowConfig    config;
    WindowCallbacks userCallbacks;
    WindowCallbacks engineCallbacks;
    bool            hooksInstalled = false;
    bool            closed   = false;
    bool            ownsGLFW = false;
    bool            vsyncDirty = false;   // setVSync pendiente de aplicar
    std::string     title;
};

namespace {

u32 s_windowCount = 0;

void glfwErrorCallback(int code, const char* desc) {
    URON_ERROR(std::string("GLFW error [") + std::to_string(code) + "]: " + desc);
}

}

Window::Window() : impl(new Impl) {}

Window::~Window() {
    destroy();
    delete impl;
}

Window::Window(Window&& o) noexcept : impl(o.impl) {
    o.impl = nullptr;
}

Window& Window::operator=(Window&& o) noexcept {
    if (this != &o) {
        destroy();
        delete impl;
        impl = o.impl;
        o.impl = nullptr;
    }
    return *this;
}

bool Window::create(const WindowConfig& cfg) {
    if (impl->handle) {
        URON_WARN("Window::create llamado dos veces. Ignorando.");
        return false;
    }

    if (s_windowCount == 0) {
        glfwSetErrorCallback(glfwErrorCallback);
        if (!glfwInit()) {
            URON_ERROR("No se pudo inicializar GLFW");
            return false;
        }
        impl->ownsGLFW = true;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, cfg.resizable ? GLFW_TRUE : GLFW_FALSE);

    GLFWmonitor* monitor = cfg.fullscreen ? glfwGetPrimaryMonitor() : nullptr;

    impl->handle = glfwCreateWindow(
        static_cast<int>(cfg.width),
        static_cast<int>(cfg.height),
        cfg.title.c_str(),
        monitor,
        nullptr
    );

    if (!impl->handle) {
        URON_ERROR("No se pudo crear la ventana GLFW");
        if (impl->ownsGLFW) {
            glfwTerminate();
            impl->ownsGLFW = false;
        }
        return false;
    }

    impl->config = cfg;
    impl->title  = cfg.title;
    impl->closed = false;
    installHooks_();
    s_windowCount++;

    URON_INFO(std::string("Ventana creada: ") + cfg.title);
    return true;
}

void Window::destroy() {
    if (!impl || !impl->handle) return;

    glfwDestroyWindow(impl->handle);
    impl->handle = nullptr;
    impl->hooksInstalled = false;

    s_windowCount--;

    if (impl->ownsGLFW && s_windowCount == 0) {
        glfwTerminate();
        impl->ownsGLFW = false;
    }
}

bool Window::shouldClose() const {
    if (!impl->handle) return true;
    return impl->closed || glfwWindowShouldClose(impl->handle);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::requestClose() {
    impl->closed = true;
    if (impl->handle) glfwSetWindowShouldClose(impl->handle, GLFW_TRUE);
}

void Window::setTitle(std::string_view title) {
    if (!impl->handle) return;
    impl->title = std::string(title);
    glfwSetWindowTitle(impl->handle, impl->title.c_str());
}

void Window::setSize(u32 w, u32 h) {
    if (!impl->handle) return;
    glfwSetWindowSize(impl->handle,
                      static_cast<int>(w),
                      static_cast<int>(h));
}

void Window::setFullscreen(bool enable) {
    if (!impl->handle) return;
    GLFWmonitor* monitor = enable ? glfwGetPrimaryMonitor() : nullptr;
    const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;

    if (mode) {
        glfwSetWindowMonitor(impl->handle, monitor, 0, 0,
                             mode->width, mode->height, mode->refreshRate);
    } else {
        glfwSetWindowMonitor(impl->handle, nullptr,
                             100, 100,
                             static_cast<int>(impl->config.width),
                             static_cast<int>(impl->config.height),
                             GLFW_DONT_CARE);
    }
}

void Window::setVSync(bool enable) {
    if (impl->config.vsync != enable) {
        impl->config.vsync = enable;
        impl->vsyncDirty   = true;   // el renderer recrea la swapchain
    }
}

std::string Window::title() const {
    return impl->title;
}

u32 Window::width() const {
    if (!impl->handle) return impl->config.width;
    int w = 0, h = 0;
    glfwGetFramebufferSize(impl->handle, &w, &h);
    return static_cast<u32>(w);
}

u32 Window::height() const {
    if (!impl->handle) return impl->config.height;
    int w = 0, h = 0;
    glfwGetFramebufferSize(impl->handle, &w, &h);
    return static_cast<u32>(h);
}

bool Window::vsync() const {
    return impl->config.vsync;
}

bool Window::takeVSyncDirty() {
    bool d = impl->vsyncDirty;
    impl->vsyncDirty = false;
    return d;
}

void Window::setCallbacks(const WindowCallbacks& cbs) {
    impl->userCallbacks = cbs;
    installHooks_();
}

void Window::setEngineCallbacks(const WindowCallbacks& cbs) {
    impl->engineCallbacks = cbs;
    installHooks_();
}

// Se instalan UNA sola vez en create(): los lambdas despachan primero al
// canal del motor (Input ve el evento antes que el codigo del usuario) y
// despues al canal usuario, de modo que setCallbacks() no pone al motor
// a ciegas ni viceversa.
void Window::installHooks_() {
    if (!impl->handle || impl->hooksInstalled) return;

    glfwSetWindowUserPointer(impl->handle, impl);

    glfwSetKeyCallback(impl->handle,
        [](GLFWwindow* w, int key, int, int action, int) {
            auto* im = static_cast<Impl*>(glfwGetWindowUserPointer(w));
            if (!im) return;
            if (im->engineCallbacks.onKey) im->engineCallbacks.onKey(key, action);
            if (im->userCallbacks.onKey)   im->userCallbacks.onKey(key, action);
        });

    glfwSetMouseButtonCallback(impl->handle,
        [](GLFWwindow* w, int button, int action, int) {
            auto* im = static_cast<Impl*>(glfwGetWindowUserPointer(w));
            if (!im) return;
            if (im->engineCallbacks.onMouseButton)
                im->engineCallbacks.onMouseButton(button, action);
            if (im->userCallbacks.onMouseButton)
                im->userCallbacks.onMouseButton(button, action);
        });

    glfwSetCursorPosCallback(impl->handle,
        [](GLFWwindow* w, double x, double y) {
            auto* im = static_cast<Impl*>(glfwGetWindowUserPointer(w));
            if (!im) return;
            const f32 fx = static_cast<f32>(x);
            const f32 fy = static_cast<f32>(y);
            if (im->engineCallbacks.onMouseMove) im->engineCallbacks.onMouseMove(fx, fy);
            if (im->userCallbacks.onMouseMove)   im->userCallbacks.onMouseMove(fx, fy);
        });

    glfwSetScrollCallback(impl->handle,
        [](GLFWwindow* w, double dx, double dy) {
            auto* im = static_cast<Impl*>(glfwGetWindowUserPointer(w));
            if (!im) return;
            const f32 fdx = static_cast<f32>(dx);
            const f32 fdy = static_cast<f32>(dy);
            if (im->engineCallbacks.onScroll) im->engineCallbacks.onScroll(fdx, fdy);
            if (im->userCallbacks.onScroll)   im->userCallbacks.onScroll(fdx, fdy);
        });

    glfwSetFramebufferSizeCallback(impl->handle,
        [](GLFWwindow* w, int width, int height) {
            auto* im = static_cast<Impl*>(glfwGetWindowUserPointer(w));
            if (!im) return;
            const u32 uw = static_cast<u32>(width);
            const u32 uh = static_cast<u32>(height);
            if (im->engineCallbacks.onResize) im->engineCallbacks.onResize(uw, uh);
            if (im->userCallbacks.onResize)   im->userCallbacks.onResize(uw, uh);
        });

    glfwSetWindowCloseCallback(impl->handle,
        [](GLFWwindow* w) {
            auto* im = static_cast<Impl*>(glfwGetWindowUserPointer(w));
            if (!im) return;
            if (im->engineCallbacks.onClose) im->engineCallbacks.onClose();
            if (im->userCallbacks.onClose)   im->userCallbacks.onClose();
        });

    impl->hooksInstalled = true;
}

void* Window::nativeHandle() const {
    return impl ? impl->handle : nullptr;
}

void* Window::nativeDisplay() const {
    // GLFW no expone el display (X11/Wayland) en ningun backend; ver
    // Window.h. Vulkan solo necesita nativeHandle().
    return nullptr;
}

bool Window::isValid() const {
    return impl && impl->handle != nullptr;
}

}