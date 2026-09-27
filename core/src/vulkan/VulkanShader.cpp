#include "VulkanShader.h"
#include "VulkanContext.h"
#include "Swapchain.h"
#include <Uron/Logger.h>
#include <fstream>

namespace Uron::Vulkan {

static std::vector<char> readFile(const char* path) {
    std::ifstream f(path, std::ios::ate | std::ios::binary);
    if (!f.is_open()) return {};
    size_t size = static_cast<size_t>(f.tellg());
    std::vector<char> buf(size);
    f.seekg(0);
    f.read(buf.data(), size);
    return buf;
}

static VkShaderModule createModule(VkDevice dev, const std::vector<char>& code) {
    if (code.empty()) return VK_NULL_HANDLE;
    VkShaderModuleCreateInfo ci{};
    ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = code.size();
    ci.pCode    = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule m = VK_NULL_HANDLE;
    vkCreateShaderModule(dev, &ci, nullptr, &m);
    return m;
}

bool VulkanShader::create(Context& ctx,
                          Swapchain& swap,
                          VkRenderPass renderPass,
                          const std::string& vertPath,
                          const std::string& fragPath,
                          const ShaderDesc& desc) {
    m_ctx = &ctx;

    auto vertCode = readFile(vertPath.c_str());
    auto fragCode = readFile(fragPath.c_str());

    VkShaderModule vert = createModule(ctx.device(), vertCode);
    VkShaderModule frag = createModule(ctx.device(), fragCode);

    if (!vert || !frag) {
        URON_ERROR("No se pudieron cargar shaders custom");
        return false;
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName  = "main";

    VkVertexInputBindingDescription binding{};
    binding.binding   = 0;
    binding.stride    = sizeof(float) * 4;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2]{};
    attrs[0].location = 0;
    attrs[0].binding  = 0;
    attrs[0].format   = VK_FORMAT_R32G32_SFLOAT;
    attrs[0].offset   = 0;
    attrs[1].location = 1;
    attrs[1].binding  = 0;
    attrs[1].format   = VK_FORMAT_R32G32_SFLOAT;
    attrs[1].offset   = sizeof(float) * 2;

    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount   = 1;
    vi.pVertexBindingDescriptions      = &binding;
    vi.vertexAttributeDescriptionCount = 2;
    vi.pVertexAttributeDescriptions    = attrs;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{};
    viewport.width    = static_cast<float>(swap.extent().width);
    viewport.height   = static_cast<float>(swap.extent().height);
    viewport.maxDepth = 1.f;

    VkRect2D scissor{};
    scissor.extent = swap.extent();

    VkPipelineViewportStateCreateInfo vp{};
    vp.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.pViewports    = &viewport;
    vp.scissorCount  = 1;
    vp.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo rast{};
    rast.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast.polygonMode = desc.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rast.lineWidth   = 1.f;
    rast.cullMode    = VK_CULL_MODE_NONE;
    rast.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAttach{};
    blendAttach.blendEnable         = desc.blending ? VK_TRUE : VK_FALSE;
    blendAttach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttach.colorBlendOp        = VK_BLEND_OP_ADD;
    blendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttach.alphaBlendOp        = VK_BLEND_OP_ADD;
    blendAttach.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT |
                                      VK_COLOR_COMPONENT_G_BIT |
                                      VK_COLOR_COMPONENT_B_BIT |
                                      VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments    = &blendAttach;

    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding         = 0;
    samplerBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo dsl{};
    dsl.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsl.bindingCount = 1;
    dsl.pBindings    = &samplerBinding;

    if (vkCreateDescriptorSetLayout(ctx.device(), &dsl, nullptr,
                                    &m_setLayout) != VK_SUCCESS) {
        URON_ERROR("vkCreateDescriptorSetLayout (custom) fallo");
        return false;
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 64;

    VkDescriptorPoolCreateInfo pci{};
    pci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pci.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pci.maxSets       = 64;
    pci.poolSizeCount = 1;
    pci.pPoolSizes    = &poolSize;

    vkCreateDescriptorPool(ctx.device(), &pci, nullptr, &m_pool);

    VkPushConstantRange push{};
    push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    push.offset     = 0;
    push.size       = sizeof(float) * 32;

    VkPipelineLayoutCreateInfo pli{};
    pli.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pli.setLayoutCount         = 1;
    pli.pSetLayouts            = &m_setLayout;
    pli.pushConstantRangeCount = 1;
    pli.pPushConstantRanges    = &push;

    if (vkCreatePipelineLayout(ctx.device(), &pli, nullptr, &m_layout) != VK_SUCCESS) {
        URON_ERROR("vkCreatePipelineLayout (custom) fallo");
        return false;
    }

    VkGraphicsPipelineCreateInfo ci{};
    ci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ci.stageCount          = 2;
    ci.pStages             = stages;
    ci.pVertexInputState   = &vi;
    ci.pInputAssemblyState = &ia;
    ci.pViewportState      = &vp;
    ci.pRasterizationState = &rast;
    ci.pMultisampleState   = &ms;
    ci.pColorBlendState    = &blend;
    ci.layout              = m_layout;
    ci.renderPass          = renderPass;
    ci.subpass             = 0;

    VkResult r = vkCreateGraphicsPipelines(ctx.device(), VK_NULL_HANDLE, 1,
                                           &ci, nullptr, &m_pipeline);

    vkDestroyShaderModule(ctx.device(), vert, nullptr);
    vkDestroyShaderModule(ctx.device(), frag, nullptr);

    if (r != VK_SUCCESS) {
        URON_ERROR("vkCreateGraphicsPipelines (custom) fallo");
        return false;
    }

    return true;
}

VkDescriptorSet VulkanShader::allocateSet(Context& ctx,
                                           VkImageView view,
                                           VkSampler sampler) {
    VkDescriptorSetAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool     = m_pool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts        = &m_setLayout;

    VkDescriptorSet set = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(ctx.device(), &ai, &set) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    VkDescriptorImageInfo img{};
    img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    img.imageView   = view;
    img.sampler     = sampler;

    VkWriteDescriptorSet write{};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet          = set;
    write.dstBinding      = 0;
    write.descriptorCount = 1;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo      = &img;

    vkUpdateDescriptorSets(ctx.device(), 1, &write, 0, nullptr);
    return set;
}

void VulkanShader::destroy(Context& ctx) {
    if (!m_ctx) return;
    if (m_pipeline)  { vkDestroyPipeline(ctx.device(), m_pipeline, nullptr); m_pipeline = VK_NULL_HANDLE; }
    if (m_layout)    { vkDestroyPipelineLayout(ctx.device(), m_layout, nullptr); m_layout = VK_NULL_HANDLE; }
    if (m_setLayout) { vkDestroyDescriptorSetLayout(ctx.device(), m_setLayout, nullptr); m_setLayout = VK_NULL_HANDLE; }
    if (m_pool)      { vkDestroyDescriptorPool(ctx.device(), m_pool, nullptr); m_pool = VK_NULL_HANDLE; }
}

}