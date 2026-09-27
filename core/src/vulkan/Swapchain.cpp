#include "Swapchain.h"
#include "VulkanContext.h"
#include <Uron/Logger.h>
#include <GLFW/glfw3.h>
#include <algorithm>

namespace Uron::Vulkan {

bool Swapchain::init(Context& ctx, GLFWwindow* window, bool vsync) {
    m_ctx    = &ctx;
    m_window = window;
    m_vsync  = vsync;
    return create();
}

void Swapchain::shutdown() {
    destroy();
}

bool Swapchain::recreate() {
    if (!m_ctx || !m_ctx->device()) return false;
    vkDeviceWaitIdle(m_ctx->device());
    destroy();
    return create();
}

bool Swapchain::create() {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_ctx->physicalDevice(),
                                              m_ctx->surface(), &caps);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_ctx->physicalDevice(),
                                         m_ctx->surface(), &formatCount, nullptr);
    if (formatCount == 0) {
        URON_ERROR("Swapchain: la superficie no expone ningun formato");
        return false;
    }
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_ctx->physicalDevice(),
                                         m_ctx->surface(), &formatCount, formats.data());

    VkSurfaceFormatKHR chosen = formats[0];
    for (auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen = f;
            break;
        }
    }
    m_format = chosen.format;

    uint32_t presentCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_ctx->physicalDevice(),
                                              m_ctx->surface(), &presentCount, nullptr);
    std::vector<VkPresentModeKHR> presents(presentCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_ctx->physicalDevice(),
                                              m_ctx->surface(), &presentCount, presents.data());

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;   // garantizado
    if (!m_vsync) {
        for (auto p : presents) {
            if (p == VK_PRESENT_MODE_MAILBOX_KHR) { presentMode = p; break; }
        }
        if (presentMode != VK_PRESENT_MODE_MAILBOX_KHR) {
            for (auto p : presents) {
                if (p == VK_PRESENT_MODE_IMMEDIATE_KHR) { presentMode = p; break; }
            }
        }
    }

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX) {
        int w = 0, h = 0;
        glfwGetFramebufferSize(m_window, &w, &h);
        extent.width  = std::clamp<uint32_t>(w, caps.minImageExtent.width,
                                                caps.maxImageExtent.width);
        extent.height = std::clamp<uint32_t>(h, caps.minImageExtent.height,
                                                caps.maxImageExtent.height);
    }
    if (extent.width == 0 || extent.height == 0) {
        // Ventana minimizada: no se puede crear; el caller reintenta.
        return false;
    }
    m_extent = extent;

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
        imageCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR ci{};
    ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface          = m_ctx->surface();
    ci.minImageCount    = imageCount;
    ci.imageFormat      = chosen.format;
    ci.imageColorSpace  = chosen.colorSpace;
    ci.imageExtent      = extent;
    ci.imageArrayLayers = 1;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    ci.imageUsage       = usage;

    uint32_t families[] = { m_ctx->graphicsFamily(), m_ctx->presentFamily() };
    if (m_ctx->graphicsFamily() != m_ctx->presentFamily()) {
        ci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices   = families;
    } else {
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    ci.preTransform   = caps.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode    = presentMode;
    ci.clipped        = VK_TRUE;
    ci.oldSwapchain   = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_ctx->device(), &ci, nullptr, &m_swapchain) != VK_SUCCESS) {
        URON_ERROR("vkCreateSwapchainKHR fallo");
        return false;
    }

    vkGetSwapchainImagesKHR(m_ctx->device(), m_swapchain, &imageCount, nullptr);
    m_images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_ctx->device(), m_swapchain, &imageCount, m_images.data());

    m_views.resize(m_images.size());
    for (size_t i = 0; i < m_images.size(); ++i) {
        VkImageViewCreateInfo vi{};
        vi.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vi.image    = m_images[i];
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vi.format   = m_format;
        vi.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        vi.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        vi.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        vi.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        vi.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        vi.subresourceRange.levelCount     = 1;
        vi.subresourceRange.layerCount     = 1;
        vkCreateImageView(m_ctx->device(), &vi, nullptr, &m_views[i]);
    }

    return true;
}

void Swapchain::destroy() {
    if (!m_ctx || !m_ctx->device()) return;
    for (auto v : m_views) vkDestroyImageView(m_ctx->device(), v, nullptr);
    m_views.clear();
    if (m_swapchain) {
        vkDestroySwapchainKHR(m_ctx->device(), m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
    m_images.clear();
}

VkResult Swapchain::acquireNextImage(VkSemaphore sem, uint32_t& outIndex) {
    return vkAcquireNextImageKHR(m_ctx->device(), m_swapchain,
                                 UINT64_MAX, sem, VK_NULL_HANDLE, &outIndex);
}

VkResult Swapchain::present(VkQueue queue, VkSemaphore waitSem, uint32_t imageIndex) {
    VkPresentInfoKHR pi{};
    pi.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores    = &waitSem;
    pi.swapchainCount     = 1;
    pi.pSwapchains        = &m_swapchain;
    pi.pImageIndices      = &imageIndex;

    return vkQueuePresentKHR(queue, &pi);
}

}