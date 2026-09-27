#include "VulkanRenderer.h"
#include "VulkanContext.h"
#include "Swapchain.h"
#include "Pipeline.h"
#include "SpritePipeline.h"
#include "VulkanImage.h"
#include <Uron/Window.h>
#include <Uron/Logger.h>
#include <Uron/render/Texture.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <unordered_map>
#include <cstring>

namespace Uron::Vulkan {

static constexpr int MAX_FRAMES = 2;

struct SpriteVertex {
    float x, y;
    float u, v;
};

struct SpritePush {
    float position[2];
    float size[2];
    float screenSize[2];
};
static_assert(sizeof(SpritePush) == 24, "SpritePush debe tener 24 bytes");

struct VulkanRenderer::Impl {
    Context     context;
    Swapchain   swapchain;
    Pipeline    pipeline;

    VkRenderPass     renderPass    = VK_NULL_HANDLE;
    VkCommandPool    commandPool   = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkFramebuffer>   framebuffers;

    std::vector<VkSemaphore> imageAvailable;
    std::vector<VkSemaphore> renderFinished;
    std::vector<VkFence>     inFlight;

    SpritePipeline* spritePipeline = nullptr;

    VkBuffer       quadBuffer       = VK_NULL_HANDLE;
    VkDeviceMemory quadBufferMemory = VK_NULL_HANDLE;

    std::vector<VulkanImage*> images;
    std::unordered_map<u64, VulkanImage*>    textureCache;
    std::unordered_map<u64, VkDescriptorSet> descriptorCache;

    uint32_t currentFrame = 0;
    uint32_t imageIndex   = 0;
    Color    clearColor{0.f, 0.f, 0.f, 1.f};
    bool     drawTriangle = true;
};

VulkanRenderer::VulkanRenderer() : m_impl(new Impl) {}

VulkanRenderer::~VulkanRenderer() {
    delete m_impl;
}

bool VulkanRenderer::createQuadBuffer() {
    SpriteVertex verts[6] = {
        { 0.f, 0.f, 0.f, 1.f },
        { 1.f, 0.f, 1.f, 1.f },
        { 1.f, 1.f, 1.f, 0.f },
        { 0.f, 0.f, 0.f, 1.f },
        { 1.f, 1.f, 1.f, 0.f },
        { 0.f, 1.f, 0.f, 0.f },
    };

    VkDeviceSize size = sizeof(verts);

    VkBufferCreateInfo bi{};
    bi.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size        = size;
    bi.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_impl->context.device(), &bi, nullptr,
                       &m_impl->quadBuffer) != VK_SUCCESS) {
        URON_ERROR("vkCreateBuffer (quad) fallo");
        return false;
    }

    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(m_impl->context.device(),
                                   m_impl->quadBuffer, &req);

    VkMemoryAllocateInfo ai{};
    ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize  = req.size;
    ai.memoryTypeIndex = m_impl->context.findMemoryType(req.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(m_impl->context.device(), &ai, nullptr,
                         &m_impl->quadBufferMemory) != VK_SUCCESS) {
        URON_ERROR("vkAllocateMemory (quad) fallo");
        return false;
    }

    vkBindBufferMemory(m_impl->context.device(), m_impl->quadBuffer,
                       m_impl->quadBufferMemory, 0);

    void* data = nullptr;
    vkMapMemory(m_impl->context.device(), m_impl->quadBufferMemory,
                0, size, 0, &data);
    std::memcpy(data, verts, sizeof(verts));
    vkUnmapMemory(m_impl->context.device(), m_impl->quadBufferMemory);

    return true;
}

bool VulkanRenderer::init(Window* window) {
    GLFWwindow* native = static_cast<GLFWwindow*>(window->nativeHandle());

    if (!m_impl->context.init(native)) return false;
    if (!m_impl->swapchain.init(m_impl->context, native)) return false;
    if (!createRenderPass()) return false;
    if (!createFramebuffers()) return false;
    if (!createCommandPool()) return false;
    if (!createCommandBuffers()) return false;
    if (!createSyncObjects()) return false;
    if (!createQuadBuffer()) return false;

    if (!m_impl->pipeline.init(m_impl->context, m_impl->swapchain,
                               m_impl->renderPass)) {
        URON_WARN("Pipeline fallo (faltan shaders .spv?)");
    }

    m_impl->spritePipeline = new SpritePipeline();
    if (!m_impl->spritePipeline->init(m_impl->context, m_impl->swapchain,
                                      m_impl->renderPass)) {
        URON_WARN("SpritePipeline fallo (faltan shaders sprite .spv?)");
    }

    return true;
}

void VulkanRenderer::shutdown() {
    if (!m_impl->context.device()) return;
    vkDeviceWaitIdle(m_impl->context.device());

    if (m_impl->quadBuffer) {
        vkDestroyBuffer(m_impl->context.device(), m_impl->quadBuffer, nullptr);
        m_impl->quadBuffer = VK_NULL_HANDLE;
    }
    if (m_impl->quadBufferMemory) {
        vkFreeMemory(m_impl->context.device(), m_impl->quadBufferMemory, nullptr);
        m_impl->quadBufferMemory = VK_NULL_HANDLE;
    }

    m_impl->textureCache.clear();
    m_impl->descriptorCache.clear();

    for (auto* img : m_impl->images) {
        if (img) {
            img->destroy(m_impl->context);
            delete img;
        }
    }
    m_impl->images.clear();

    if (m_impl->spritePipeline) {
        m_impl->spritePipeline->shutdown();
        delete m_impl->spritePipeline;
        m_impl->spritePipeline = nullptr;
    }

    for (auto f : m_impl->inFlight)       vkDestroyFence(m_impl->context.device(), f, nullptr);
    for (auto s : m_impl->renderFinished) vkDestroySemaphore(m_impl->context.device(), s, nullptr);
    for (auto s : m_impl->imageAvailable) vkDestroySemaphore(m_impl->context.device(), s, nullptr);
    m_impl->inFlight.clear();
    m_impl->renderFinished.clear();
    m_impl->imageAvailable.clear();

    if (m_impl->commandPool)
        vkDestroyCommandPool(m_impl->context.device(), m_impl->commandPool, nullptr);

    for (auto fb : m_impl->framebuffers)
        vkDestroyFramebuffer(m_impl->context.device(), fb, nullptr);
    m_impl->framebuffers.clear();

    if (m_impl->renderPass)
        vkDestroyRenderPass(m_impl->context.device(), m_impl->renderPass, nullptr);

    m_impl->pipeline.shutdown();
    m_impl->swapchain.shutdown();
    m_impl->context.shutdown();
}

bool VulkanRenderer::createRenderPass() {
    VkAttachmentDescription color{};
    color.format         = m_impl->swapchain.format();
    color.samples        = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription sub{};
    sub.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments    = &colorRef;

    VkSubpassDependency dep{};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo ci{};
    ci.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    ci.attachmentCount = 1;
    ci.pAttachments    = &color;
    ci.subpassCount    = 1;
    ci.pSubpasses      = &sub;
    ci.dependencyCount = 1;
    ci.pDependencies   = &dep;

    return vkCreateRenderPass(m_impl->context.device(), &ci, nullptr,
                              &m_impl->renderPass) == VK_SUCCESS;
}

bool VulkanRenderer::createFramebuffers() {
    auto& views = m_impl->swapchain.views();
    m_impl->framebuffers.resize(views.size());

    for (size_t i = 0; i < views.size(); ++i) {
        VkImageView attachments[] = { views[i] };
        VkFramebufferCreateInfo ci{};
        ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        ci.renderPass      = m_impl->renderPass;
        ci.attachmentCount = 1;
        ci.pAttachments    = attachments;
        ci.width           = m_impl->swapchain.extent().width;
        ci.height          = m_impl->swapchain.extent().height;
        ci.layers          = 1;

        if (vkCreateFramebuffer(m_impl->context.device(), &ci, nullptr,
                                &m_impl->framebuffers[i]) != VK_SUCCESS) {
            return false;
        }
    }
    return true;
}

bool VulkanRenderer::createCommandPool() {
    VkCommandPoolCreateInfo ci{};
    ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ci.queueFamilyIndex = m_impl->context.graphicsFamily();
    return vkCreateCommandPool(m_impl->context.device(), &ci, nullptr,
                               &m_impl->commandPool) == VK_SUCCESS;
}

bool VulkanRenderer::createCommandBuffers() {
    m_impl->commandBuffers.resize(MAX_FRAMES);
    VkCommandBufferAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool        = m_impl->commandPool;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = MAX_FRAMES;
    return vkAllocateCommandBuffers(m_impl->context.device(), &ai,
                                    m_impl->commandBuffers.data()) == VK_SUCCESS;
}

bool VulkanRenderer::createSyncObjects() {
    m_impl->imageAvailable.resize(MAX_FRAMES);
    m_impl->renderFinished.resize(MAX_FRAMES);
    m_impl->inFlight.resize(MAX_FRAMES);

    VkSemaphoreCreateInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fi{};
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES; ++i) {
        if (vkCreateSemaphore(m_impl->context.device(), &si, nullptr,
                              &m_impl->imageAvailable[i]) != VK_SUCCESS) return false;
        if (vkCreateSemaphore(m_impl->context.device(), &si, nullptr,
                              &m_impl->renderFinished[i]) != VK_SUCCESS) return false;
        if (vkCreateFence(m_impl->context.device(), &fi, nullptr,
                          &m_impl->inFlight[i]) != VK_SUCCESS) return false;
    }
    return true;
}

void VulkanRenderer::beginFrame() {
    auto dev = m_impl->context.device();
    vkWaitForFences(dev, 1, &m_impl->inFlight[m_impl->currentFrame],
                    VK_TRUE, UINT64_MAX);

    if (!m_impl->swapchain.acquireNextImage(
            m_impl->imageAvailable[m_impl->currentFrame], m_impl->imageIndex)) {
        return;
    }

    vkResetFences(dev, 1, &m_impl->inFlight[m_impl->currentFrame]);

    auto cmd = m_impl->commandBuffers[m_impl->currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &bi);

    VkClearValue clear{};
    clear.color = {{ m_impl->clearColor.r, m_impl->clearColor.g,
                     m_impl->clearColor.b, m_impl->clearColor.a }};

    VkRenderPassBeginInfo rp{};
    rp.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp.renderPass        = m_impl->renderPass;
    rp.framebuffer       = m_impl->framebuffers[m_impl->imageIndex];
    rp.renderArea.offset = {0, 0};
    rp.renderArea.extent = m_impl->swapchain.extent();
    rp.clearValueCount   = 1;
    rp.pClearValues      = &clear;

    vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);

    if (m_impl->drawTriangle && m_impl->pipeline.handle()) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          m_impl->pipeline.handle());
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }
}

void VulkanRenderer::clear(const Color& c) {
    m_impl->clearColor = c;
}

void VulkanRenderer::endFrame() {
    auto cmd = m_impl->commandBuffers[m_impl->currentFrame];

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo si{};
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSems[]  = { m_impl->imageAvailable[m_impl->currentFrame] };
    VkPipelineStageFlags ws[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    si.waitSemaphoreCount   = 1;
    si.pWaitSemaphores      = waitSems;
    si.pWaitDstStageMask    = ws;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &cmd;
    VkSemaphore sigSems[]   = { m_impl->renderFinished[m_impl->currentFrame] };
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores    = sigSems;

    vkQueueSubmit(m_impl->context.graphicsQueue(), 1, &si,
                  m_impl->inFlight[m_impl->currentFrame]);

    m_impl->swapchain.present(m_impl->context.presentQueue(),
                              m_impl->renderFinished[m_impl->currentFrame],
                              m_impl->imageIndex);

    m_impl->currentFrame = (m_impl->currentFrame + 1) % MAX_FRAMES;
}

void VulkanRenderer::drawSprite(const Texture& tex, const Mat4& transform) {
    static int calls = 0;
    if (calls < 3) {
        URON_INFO("drawSprite llamado (llamada #" + std::to_string(calls + 1) + ")");
        calls++;
    }

    if (!tex.isValid()) {
        URON_WARN("drawSprite: textura invalida");
        return;
    }
    if (!m_impl->spritePipeline || !m_impl->spritePipeline->handle()) {
        URON_WARN("drawSprite: spritePipeline no disponible");
        return;
    }

    auto cmd = m_impl->commandBuffers[m_impl->currentFrame];

    u64 key = tex.handle();

    VulkanImage* img = nullptr;
    auto itImg = m_impl->textureCache.find(key);
    if (itImg != m_impl->textureCache.end()) {
        img = itImg->second;
    } else {
        img = new VulkanImage();
        if (!img->create(m_impl->context, tex.width(), tex.height(),
                         tex.pixels(), 4)) {
            delete img;
            return;
        }
        m_impl->textureCache[key] = img;
        m_impl->images.push_back(img);
    }

    VkDescriptorSet set = VK_NULL_HANDLE;
    auto itSet = m_impl->descriptorCache.find(key);
    if (itSet != m_impl->descriptorCache.end()) {
        set = itSet->second;
    } else {
        set = m_impl->spritePipeline->allocateSet(m_impl->context,
                                                   img->view(),
                                                   img->sampler());
        if (set == VK_NULL_HANDLE) return;
        m_impl->descriptorCache[key] = set;
    }

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_impl->quadBuffer, &offset);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      m_impl->spritePipeline->handle());
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_impl->spritePipeline->layout(),
                            0, 1, &set, 0, nullptr);

    SpritePush push{};
    push.position[0] = transform.m[3][0];
    push.position[1] = transform.m[3][1];
    push.size[0] = transform.m[0][0];
    push.size[1] = transform.m[1][1];
    push.screenSize[0] = static_cast<float>(m_impl->swapchain.extent().width);
    push.screenSize[1] = static_cast<float>(m_impl->swapchain.extent().height);

    vkCmdPushConstants(cmd, m_impl->spritePipeline->layout(),
                       VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(SpritePush), &push);

    vkCmdDraw(cmd, 6, 1, 0, 0);
}

void VulkanRenderer::waitIdle() {
    if (m_impl->context.device()) {
        vkDeviceWaitIdle(m_impl->context.device());
    }
}

u32 VulkanRenderer::width() const {
    return m_impl->swapchain.extent().width;
}

u32 VulkanRenderer::height() const {
    return m_impl->swapchain.extent().height;
}

VulkanImage* VulkanRenderer::createImage(u32 width, u32 height, const u8* pixels) {
    if (!pixels) return nullptr;
    auto* img = new VulkanImage();
    if (!img->create(m_impl->context, width, height, pixels, 4)) {
        delete img;
        return nullptr;
    }
    m_impl->images.push_back(img);
    return img;
}

void VulkanRenderer::shutdownVulkan() {
    shutdown();
}

}