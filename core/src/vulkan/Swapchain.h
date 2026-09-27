#pragma once
#include <vulkan/vulkan.h>
#include <vector>

struct GLFWwindow;

namespace Uron::Vulkan {

class Context;

class Swapchain {
public:
    bool init(Context& ctx, GLFWwindow* window);
    void shutdown();

    bool acquireNextImage(VkSemaphore sem, uint32_t& outIndex);
    bool present(VkQueue queue, VkSemaphore waitSem, uint32_t imageIndex);

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
    VkSwapchainKHR   m_swapchain = VK_NULL_HANDLE;
    VkFormat         m_format    = VK_FORMAT_UNDEFINED;
    VkExtent2D       m_extent{};
    std::vector<VkImage>     m_images;
    std::vector<VkImageView> m_views;
};

}