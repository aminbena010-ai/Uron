#pragma once
#include <vulkan/vulkan.h>

namespace Uron::Vulkan {

class Context;
class Swapchain;

class SpritePipeline {
public:
    bool init(Context& ctx, Swapchain& swap, VkRenderPass renderPass);
    void shutdown();

    VkPipeline            handle()    const { return m_pipeline;  }
    VkPipelineLayout      layout()    const { return m_layout;    }
    VkDescriptorSetLayout setLayout() const { return m_setLayout; }
    VkDescriptorPool      pool()      const { return m_pool;      }

    VkDescriptorSet allocateSet(Context& ctx, VkImageView view, VkSampler sampler);

private:
    Context*              m_ctx       = nullptr;
    VkPipeline            m_pipeline  = VK_NULL_HANDLE;
    VkPipelineLayout      m_layout    = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;
    VkDescriptorPool      m_pool      = VK_NULL_HANDLE;
};

}