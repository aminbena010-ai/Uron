#pragma once
#include <vulkan/vulkan.h>
#include <vector>

struct GLFWwindow;

namespace Uron::Vulkan {

class Context;

class Swapchain {
public:
    // vsync=true  => present mode FIFO (siempre disponible).
    // vsync=false => MAILBOX si existe, si no IMMEDIATE, si no FIFO.
    bool init(Context& ctx, GLFWwindow* window, bool vsync);
    void shutdown();

    // Destruye y crea de nuevo (tras resize o cambio de vsync).
    // Devuelve false si la superficie esta en extent 0 (ventana minimizada):
    // en ese caso no se destruye la swapchain actual.
    bool recreate();

    VkResult acquireNextImage(VkSemaphore sem, uint32_t& outIndex);
    VkResult present(VkQueue queue, VkSemaphore waitSem, uint32_t imageIndex);

    VkSwapchainKHR handle()  const { return m_swapchain; }
    VkFormat       format()  const { return m_format; }
    VkExtent2D     extent()  const { return m_extent; }
    const std::vector<VkImage>&     images() const { return m_images; }
    const std::vector<VkImageView>& views()  const { return m_views;  }

private:
    bool create();
    void destroy();

    Context*         m_ctx       = nullptr;
    GLFWwindow*      m_window    = nullptr;
    bool             m_vsync     = true;
    VkSwapchainKHR   m_swapchain = VK_NULL_HANDLE;
    VkFormat         m_format    = VK_FORMAT_UNDEFINED;
    VkExtent2D       m_extent{};
    std::vector<VkImage>     m_images;
    std::vector<VkImageView> m_views;
};

}