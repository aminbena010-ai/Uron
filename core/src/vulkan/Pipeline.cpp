#include "Pipeline.h"
#include "VulkanContext.h"
#include "Swapchain.h"
#include <Uron/Logger.h>
#include <vector>
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

bool Pipeline::init(Context& ctx, Swapchain& swap, VkRenderPass renderPass) {
    m_ctx = &ctx;
    m_swap = &swap;

    auto vertCode = readFile("shaders/triangle.vert.spv");
    auto fragCode = readFile("shaders/triangle.frag.spv");

    VkShaderModule vert = createModule(ctx.device(), vertCode);
    VkShaderModule frag = createModule(ctx.device(), fragCode);

    if (!vert || !frag) {
        URON_ERROR("No se pudieron cargar shaders .spv");
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

    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

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
    rast.polygonMode = VK_POLYGON_MODE_FILL;
    rast.lineWidth   = 1.f;
    rast.cullMode    = VK_CULL_MODE_BACK_BIT;
    rast.frontFace   = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAttach{};
    blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                 VK_COLOR_COMPONENT_G_BIT |
                                 VK_COLOR_COMPONENT_B_BIT |
                                 VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments    = &blendAttach;

    VkPipelineLayoutCreateInfo pli{};
    pli.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    vkCreatePipelineLayout(ctx.device(), &pli, nullptr, &m_layout);

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
        URON_ERROR("vkCreateGraphicsPipelines fallo");
        return false;
    }
    return true;
}

void Pipeline::shutdown() {
    if (!m_ctx || !m_ctx->device()) return;
    if (m_pipeline) vkDestroyPipeline(m_ctx->device(), m_pipeline, nullptr);
    if (m_layout)   vkDestroyPipelineLayout(m_ctx->device(), m_layout, nullptr);
    m_pipeline = VK_NULL_HANDLE;
    m_layout   = VK_NULL_HANDLE;
}

}