#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>

namespace Uron::Vulkan {

class Context;

class VulkanImage {
public:
    bool create(Context& ctx,
                uint32_t width,
                uint32_t height,
                const uint8_t* pixels,
                uint32_t channels = 4);

    void destroy(Context& ctx);

    VkImage     image()   const { return m_image;   }
    VkImageView view()    const { return m_view;    }
    VkSampler   sampler() const { return m_sampler; }
    VkDeviceMemory memory() const { return m_memory; }

    uint32_t width()  const { return m_width;  }
    uint32_t height() const { return m_height; }

private:
    VkImage        m_image   = VK_NULL_HANDLE;
    VkImageView    m_view    = VK_NULL_HANDLE;
    VkSampler      m_sampler = VK_NULL_HANDLE;
    VkDeviceMemory m_memory  = VK_NULL_HANDLE;
    uint32_t       m_width   = 0;
    uint32_t       m_height  = 0;
};

}