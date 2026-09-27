#pragma once
#include <vulkan/vulkan.h>
#include <Uron/render/Shader.h>
#include <string>
#include <vector>

namespace Uron::Vulkan {

class Context;
class Swapchain;

class VulkanShader {
public:
    bool create(Context& ctx,
                Swapchain& swap,
                VkRenderPass renderPass,
                const std::string& vertPath,
                const std::string& fragPath,
                const ShaderDesc& desc);

    void destroy(Context& ctx);

    VkPipeline       pipeline() const { return m_pipeline; }
    VkPipelineLayout layout()   const { return m_layout;   }

    VkDescriptorSet allocateSet(Context& ctx,
                                VkImageView view,
                                VkSampler sampler);

private:
    Context*              m_ctx       = nullptr;
    VkPipeline            m_pipeline  = VK_NULL_HANDLE;
    VkPipelineLayout      m_layout    = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;
    VkDescriptorPool      m_pool      = VK_NULL_HANDLE;
};

}