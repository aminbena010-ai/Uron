#pragma once
#include <vulkan/vulkan.h>

namespace Uron::Vulkan {

class Context;
class Swapchain;

class Pipeline {
public:
    bool init(Context& ctx, Swapchain& swap, VkRenderPass renderPass);
    void shutdown();

    VkPipeline       handle() const { return m_pipeline; }
    VkPipelineLayout layout() const { return m_layout;   }

private:
    Context*         m_ctx      = nullptr;
    Swapchain*       m_swap     = nullptr;
    VkPipeline       m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_layout   = VK_NULL_HANDLE;
};

}