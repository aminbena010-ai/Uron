#include <Uron/Window.h>
#include <Uron/Logger.h>
#include <GLFW/glfw3.h>
#include <string>

namespace Uron {

struct Window::Impl {
    GLFWwindow*     handle   = nullptr;
    WindowConfig    config;
    WindowCallbacks callbacks;
    bool            closed   = false;
    bool            ownsGLFW = false;
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
    s_windowCount++;

    URON_INFO(std::string("Ventana creada: ") + cfg.title);
    return true;
}

void Window::destroy() {
    if (!impl || !impl->handle) return;

    glfwDestroyWindow(impl->handle);
    impl->handle = nullptr;

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
    impl->config.vsync = enable;
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

void Window::setCallbacks(const WindowCallbacks& cbs) {
    impl->callbacks = cbs;
    if (!impl->handle) return;

    glfwSetWindowUserPointer(impl->handle, impl);

    if (cbs.onKey) {
        glfwSetKeyCallback(impl->handle,
            [](GLFWwindow* w, int key, int, int action, int) {
                auto* im = static_cast<Impl*>(glfwGetWindowUserPointer(w));
                if (im && im->callbacks.onKey) im->callbacks.onKey(key, action);
            });
    }

    if (cbs.onMouseMove) {
        glfwSetCursorPosCallback(impl->handle,
            [](GLFWwindow* w, double x, double y) {
                auto* im = static_cast<Impl*>(glfwGetWindowUserPointer(w));
                if (im && im->callbacks.onMouseMove)
                    im->callbacks.onMouseMove(static_cast<f32>(x),
                                              static_cast<f32>(y));
            });
    }

    if (cbs.onResize) {
        glfwSetFramebufferSizeCallback(impl->handle,
            [](GLFWwindow* w, int width, int height) {
                auto* im = static_cast<Impl*>(glfwGetWindowUserPointer(w));
                if (im && im->callbacks.onResize)
                    im->callbacks.onResize(static_cast<u32>(width),
                                           static_cast<u32>(height));
            });
    }

    if (cbs.onClose) {
        glfwSetWindowCloseCallback(impl->handle,
            [](GLFWwindow* w) {
                auto* im = static_cast<Impl*>(glfwGetWindowUserPointer(w));
                if (im && im->callbacks.onClose) im->callbacks.onClose();
            });
    }
}

void* Window::nativeHandle() const {
    return impl ? impl->handle : nullptr;
}

void* Window::nativeDisplay() const {
    return nullptr;
}

bool Window::isValid() const {
    return impl && impl->handle != nullptr;
}

}