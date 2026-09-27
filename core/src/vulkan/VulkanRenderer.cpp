#include "VulkanRenderer.h"
#include "VulkanContext.h"
#include "Swapchain.h"
#include "Pipeline.h"
#include "SpritePipeline.h"
#include "VulkanImage.h"
#include "VulkanShader.h"
#include "render/ShaderInternal.h"
#include <Uron/Window.h>
#include <Uron/Logger.h>
#include <Uron/render/Texture.h>
#include <Uron/render/Shader.h>
#include <Uron/render/Mesh.h>
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

// Afín 2D de quad unitario → pixeles: pix = (a*x + c*y + e, b*x + d*y + f)
struct SpritePush {
    float row0[4];      // a b c d
    float row1[4];      // e f screenW screenH
    float tint[4];      // rgba
};
static_assert(sizeof(SpritePush) == SPRITE_PUSH_BYTES,
              "SpritePush debe tener 48 bytes");

// CLAUDE.md §5: shaders custom = 128 bytes (32 floats) = sprite + uniforms
struct CustomPush {
    SpritePush sprite;
    u8         uniforms[UNIFORM_PUSH_BYTES];
};
static_assert(sizeof(CustomPush) == CUSTOM_PUSH_BYTES,
              "CustomPush debe tener 128 bytes");
static_assert(sizeof(CustomPush) == sizeof(float) * 32,
              "CustomPush debe ocupar 32 floats");

// MVP para drawMesh (push constante VERTEX del pipeline de mallas)
struct MeshPush {
    float mvp[16];
};
static_assert(sizeof(MeshPush) == 64, "MVP push = 64 bytes");

struct VulkanRenderer::Impl {
    // Pipeline custom por Shader::handle(); shader == nullptr => la creacion
    // fallo y se usa el pipeline por defecto (no se reintenta cada frame).
    // owner = ShaderData que origino la entrada (detecta recargas: BUG-018).
    struct CustomShader {
        VulkanShader* shader = nullptr;
        bool attempted = false;
        ShaderData* owner = nullptr;
        std::unordered_map<u64, VkDescriptorSet> sets;
    };

    Context     context;
    Swapchain   swapchain;
    Pipeline    pipeline;

    // Profundidad compartida por todos los frames (una sola imagen)
    VkImage        depthImage  = VK_NULL_HANDLE;
    VkImageView    depthView   = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory = VK_NULL_HANDLE;
    VkFormat       depthFormat = VK_FORMAT_D32_SFLOAT;

    // Proyección*visión activa para drawMesh (identidad = MVP ya completo)
    Mat4 viewProj = Mat4::identity();

    // Buffers GPU por malla; el puntero identifica la malla y una firma
    // (hash) valida que el contenido no haya cambiado entre frames.
    struct MeshGpu {
        VkBuffer       vbuf = VK_NULL_HANDLE;
        VkDeviceMemory vmem = VK_NULL_HANDLE;
        VkBuffer       ibuf = VK_NULL_HANDLE;
        VkDeviceMemory imem = VK_NULL_HANDLE;
        u32            indexCount = 0;
        u64            sig = 0;
    };
    std::unordered_map<const Mesh*, MeshGpu> meshCache;
    bool warnedMeshPipeline = false;

    Window* window          = nullptr;   // para leer/tomar flags (vsync)
    bool    frameValid      = false;     // beginFrame completo (BUG-038)
    bool    swapchainDirty  = false;     // resize o cambio de vsync pendiente

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
    std::unordered_map<u64, CustomShader>    customShaders;
    // Entradas retiradas por recarga/destroy de shader; se destruyen en el
    // proximo beginFrame con deviceWaitIdle (BUG-018).
    std::vector<CustomShader>                pendingDestroy;

    uint32_t currentFrame = 0;
    uint32_t imageIndex   = 0;
    Color    clearColor{0.f, 0.f, 0.f, 1.f};
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

    if (ai.memoryTypeIndex == UINT32_MAX) {
        vkDestroyBuffer(m_impl->context.device(), m_impl->quadBuffer, nullptr);
        m_impl->quadBuffer = VK_NULL_HANDLE;
        URON_ERROR("findMemoryType fallo (quad)");
        return false;
    }

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
    m_impl->window     = window;

    if (!m_impl->context.init(native)) return false;
    if (!m_impl->swapchain.init(m_impl->context, native, window->vsync()))
        return false;
    if (!createDepthResources()) return false;
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

    for (auto& kv : m_impl->meshCache) {
        auto dev = m_impl->context.device();
        if (kv.second.vbuf) vkDestroyBuffer(dev, kv.second.vbuf, nullptr);
        if (kv.second.vmem) vkFreeMemory(dev, kv.second.vmem, nullptr);
        if (kv.second.ibuf) vkDestroyBuffer(dev, kv.second.ibuf, nullptr);
        if (kv.second.imem) vkFreeMemory(dev, kv.second.imem, nullptr);
    }
    m_impl->meshCache.clear();

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

    for (auto& kv : m_impl->customShaders) {
        if (kv.second.shader) {
            kv.second.shader->destroy(m_impl->context);
            delete kv.second.shader;
        }
    }
    m_impl->customShaders.clear();

    for (auto& e : m_impl->pendingDestroy) {
        if (e.shader) {
            e.shader->destroy(m_impl->context);
            delete e.shader;
        }
    }
    m_impl->pendingDestroy.clear();

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

    if (m_impl->depthView)
        vkDestroyImageView(m_impl->context.device(), m_impl->depthView, nullptr);
    if (m_impl->depthImage)
        vkDestroyImage(m_impl->context.device(), m_impl->depthImage, nullptr);
    if (m_impl->depthMemory)
        vkFreeMemory(m_impl->context.device(), m_impl->depthMemory, nullptr);
    m_impl->depthView   = VK_NULL_HANDLE;
    m_impl->depthImage  = VK_NULL_HANDLE;
    m_impl->depthMemory = VK_NULL_HANDLE;

    if (m_impl->renderPass)
        vkDestroyRenderPass(m_impl->context.device(), m_impl->renderPass, nullptr);

    m_impl->pipeline.shutdown();
    m_impl->swapchain.shutdown();
    m_impl->context.shutdown();
}

bool VulkanRenderer::createDepthResources() {
    auto dev = m_impl->context.device();
    auto ext = m_impl->swapchain.extent();

    VkImageCreateInfo ii{};
    ii.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ii.imageType     = VK_IMAGE_TYPE_2D;
    ii.extent.width  = ext.width;
    ii.extent.height = ext.height;
    ii.extent.depth  = 1;
    ii.mipLevels     = 1;
    ii.arrayLayers   = 1;
    ii.format        = m_impl->depthFormat;
    ii.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ii.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ii.samples       = VK_SAMPLE_COUNT_1_BIT;
    ii.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(dev, &ii, nullptr, &m_impl->depthImage) != VK_SUCCESS) {
        URON_ERROR("vkCreateImage (depth) fallo");
        return false;
    }

    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(dev, m_impl->depthImage, &req);

    VkMemoryAllocateInfo ai{};
    ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize  = req.size;
    ai.memoryTypeIndex = m_impl->context.findMemoryType(
        req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (ai.memoryTypeIndex == UINT32_MAX) {
        ai.memoryTypeIndex = m_impl->context.findMemoryType(
            req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }
    if (ai.memoryTypeIndex == UINT32_MAX) {
        URON_ERROR("findMemoryType fallo (depth)");
        return false;
    }

    if (vkAllocateMemory(dev, &ai, nullptr, &m_impl->depthMemory)
        != VK_SUCCESS) {
        URON_ERROR("vkAllocateMemory (depth) fallo");
        return false;
    }
    vkBindImageMemory(dev, m_impl->depthImage, m_impl->depthMemory, 0);

    VkImageViewCreateInfo vi{};
    vi.sType                          = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image                          = m_impl->depthImage;
    vi.viewType                       = VK_IMAGE_VIEW_TYPE_2D;
    vi.format                         = m_impl->depthFormat;
    vi.subresourceRange.aspectMask    = VK_IMAGE_ASPECT_DEPTH_BIT;
    vi.subresourceRange.baseMipLevel  = 0;
    vi.subresourceRange.levelCount    = 1;
    vi.subresourceRange.baseArrayLayer = 0;
    vi.subresourceRange.layerCount    = 1;

    if (vkCreateImageView(dev, &vi, nullptr, &m_impl->depthView)
        != VK_SUCCESS) {
        URON_ERROR("vkCreateImageView (depth) fallo");
        return false;
    }
    return true;
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

    VkAttachmentDescription depth{};
    depth.format         = m_impl->depthFormat;
    depth.samples        = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 1;
    depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription sub{};
    sub.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount    = 1;
    sub.pColorAttachments       = &colorRef;
    sub.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dep{};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = { color, depth };

    VkRenderPassCreateInfo ci{};
    ci.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    ci.attachmentCount = 2;
    ci.pAttachments    = attachments;
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
        VkImageView attachments[] = { views[i], m_impl->depthView };
        VkFramebufferCreateInfo ci{};
        ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        ci.renderPass      = m_impl->renderPass;
        ci.attachmentCount = 2;
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
    m_impl->inFlight.resize(MAX_FRAMES);
    // renderFinished: UNO POR IMAGEN de swapchain. Senalar el mismo por
    // frame hacia que el present anterior pudiera seguir usandolo
    // (error de validacion "semaphore still in use by swapchain").
    m_impl->renderFinished.resize(m_impl->swapchain.images().size());

    VkSemaphoreCreateInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fi{};
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES; ++i) {
        if (vkCreateSemaphore(m_impl->context.device(), &si, nullptr,
                              &m_impl->imageAvailable[i]) != VK_SUCCESS) return false;
        if (vkCreateFence(m_impl->context.device(), &fi, nullptr,
                          &m_impl->inFlight[i]) != VK_SUCCESS) return false;
    }
    for (size_t i = 0; i < m_impl->renderFinished.size(); ++i) {
        if (vkCreateSemaphore(m_impl->context.device(), &si, nullptr,
                              &m_impl->renderFinished[i]) != VK_SUCCESS)
            return false;
    }
    return true;
}

void VulkanRenderer::beginFrame() {
    auto dev = m_impl->context.device();
    m_impl->frameValid = false;

    // Pipelines custom retirados (recarga/destroy de shader): destruir con
    // el dispositivo quieto para no tirar un pipeline en vuelo (BUG-018).
    if (!m_impl->pendingDestroy.empty()) {
        vkDeviceWaitIdle(dev);
        for (auto& e : m_impl->pendingDestroy) {
            if (e.shader) {
                e.shader->destroy(m_impl->context);
                delete e.shader;
            }
        }
        m_impl->pendingDestroy.clear();
    }

    // BUG-016: setVSync() en caliente => recrear con el present mode nuevo.
    if (m_impl->window && m_impl->window->takeVSyncDirty()) {
        m_impl->swapchainDirty = true;
    }
    // BUG-005: resize / OUT_OF_DATE pendiente de aplicar.
    if (m_impl->swapchainDirty) {
        if (!recreateSwapchain()) return;   // p.ej. minimizado (extent 0)
        m_impl->swapchainDirty = false;
    }

    vkWaitForFences(dev, 1, &m_impl->inFlight[m_impl->currentFrame],
                    VK_TRUE, UINT64_MAX);

    // BUG-005: los resultados de acquire se propagan al present/next frame.
    VkResult ar = m_impl->swapchain.acquireNextImage(
        m_impl->imageAvailable[m_impl->currentFrame], m_impl->imageIndex);
    if (ar == VK_ERROR_OUT_OF_DATE_KHR) {
        m_impl->swapchainDirty = true;
        return;
    }
    if (ar != VK_SUCCESS && ar != VK_SUBOPTIMAL_KHR) {
        return;
    }
    if (ar == VK_SUBOPTIMAL_KHR) {
        m_impl->swapchainDirty = true;   // se recrea en el proximo frame
    }

    vkResetFences(dev, 1, &m_impl->inFlight[m_impl->currentFrame]);

    auto cmd = m_impl->commandBuffers[m_impl->currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &bi);

    VkClearValue clears[2]{};
    clears[0].color = {{ m_impl->clearColor.r, m_impl->clearColor.g,
                         m_impl->clearColor.b, m_impl->clearColor.a }};
    clears[1].depthStencil = { 1.f, 0 };

    VkRenderPassBeginInfo rp{};
    rp.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp.renderPass        = m_impl->renderPass;
    rp.framebuffer       = m_impl->framebuffers[m_impl->imageIndex];
    rp.renderArea.offset = {0, 0};
    rp.renderArea.extent = m_impl->swapchain.extent();
    rp.clearValueCount   = 2;
    rp.pClearValues      = clears;

    vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);

    // Viewport/scissor dinamicos: los pipelines no guardan tamaño fijo,
    // asi que un resize no obliga a recrear pipelines.
    VkExtent2D e = m_impl->swapchain.extent();
    VkViewport vp{};
    vp.x        = 0.f;
    vp.y        = 0.f;
    vp.width    = static_cast<float>(e.width);
    vp.height   = static_cast<float>(e.height);
    vp.minDepth = 0.f;
    vp.maxDepth = 1.f;
    VkRect2D sc{{0, 0}, e};
    vkCmdSetViewport(cmd, 0, 1, &vp);
    vkCmdSetScissor(cmd, 0, 1, &sc);

    m_impl->frameValid = true;
}

bool VulkanRenderer::recreateSwapchain() {
    if (!m_impl->swapchain.recreate()) return false;

    // El numero de imagenes puede cambiar: rehacer renderFinished.
    for (auto s : m_impl->renderFinished)
        vkDestroySemaphore(m_impl->context.device(), s, nullptr);
    m_impl->renderFinished.clear();
    VkSemaphoreCreateInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    m_impl->renderFinished.resize(m_impl->swapchain.images().size());
    for (size_t i = 0; i < m_impl->renderFinished.size(); ++i) {
        if (vkCreateSemaphore(m_impl->context.device(), &si, nullptr,
                              &m_impl->renderFinished[i]) != VK_SUCCESS) {
            return false;
        }
    }

    for (auto fb : m_impl->framebuffers)
        vkDestroyFramebuffer(m_impl->context.device(), fb, nullptr);
    m_impl->framebuffers.clear();

    if (m_impl->depthView)
        vkDestroyImageView(m_impl->context.device(), m_impl->depthView, nullptr);
    if (m_impl->depthImage)
        vkDestroyImage(m_impl->context.device(), m_impl->depthImage, nullptr);
    if (m_impl->depthMemory)
        vkFreeMemory(m_impl->context.device(), m_impl->depthMemory, nullptr);
    m_impl->depthView   = VK_NULL_HANDLE;
    m_impl->depthImage  = VK_NULL_HANDLE;
    m_impl->depthMemory = VK_NULL_HANDLE;

    return createDepthResources() && createFramebuffers();
}

void VulkanRenderer::clear(const Color& c) {
    m_impl->clearColor = c;
}

void VulkanRenderer::endFrame() {
    // BUG-038: si beginFrame no completo (acquire fallo/recreate), no hay
    // frame que submitir; imageIndex podia quedar obsoleto.
    if (!m_impl->frameValid) return;
    m_impl->frameValid = false;

    auto cmd = m_impl->commandBuffers[m_impl->currentFrame];

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo si{};
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSems[]  = { m_impl->imageAvailable[m_impl->currentFrame] };
    // La profundidad se limpia/eshardea en EARLY_FRAGMENT_TESTS: esperar
    // tambien ahi evita escribir depth antes de que la imagen este lista.
    VkPipelineStageFlags ws[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
    };
    si.waitSemaphoreCount   = 1;
    si.pWaitSemaphores      = waitSems;
    si.pWaitDstStageMask    = ws;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &cmd;
    // Un semaforo por imagen: el del swapchain que vamos a presentar.
    VkSemaphore sigSems[]   = { m_impl->renderFinished[m_impl->imageIndex] };
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores    = sigSems;

    vkQueueSubmit(m_impl->context.graphicsQueue(), 1, &si,
                  m_impl->inFlight[m_impl->currentFrame]);

    // BUG-005: OUT_OF_DATE/SUBOPTIMAL en present => recrear proximo frame.
    VkResult pr = m_impl->swapchain.present(
        m_impl->context.presentQueue(),
        m_impl->renderFinished[m_impl->imageIndex],
        m_impl->imageIndex);
    if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR) {
        m_impl->swapchainDirty = true;
    }

    m_impl->currentFrame = (m_impl->currentFrame + 1) % MAX_FRAMES;
}

void VulkanRenderer::drawSprite(const Texture& tex, const Mat4& transform,
                                Shader* shader, const Color& tint) {
    if (!tex.isValid()) {
        URON_WARN("drawSprite: textura invalida");
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

    SpritePush push{};
    push.row0[0] = transform.m[0][0];
    push.row0[1] = transform.m[0][1];
    push.row0[2] = transform.m[1][0];
    push.row0[3] = transform.m[1][1];
    push.row1[0] = transform.m[3][0];
    push.row1[1] = transform.m[3][1];
    push.row1[2] = static_cast<float>(m_impl->swapchain.extent().width);
    push.row1[3] = static_cast<float>(m_impl->swapchain.extent().height);
    push.tint[0] = tint.r;
    push.tint[1] = tint.g;
    push.tint[2] = tint.b;
    push.tint[3] = tint.a;

    if (shader && shader->isValid() &&
        drawSpriteCustom(cmd, img, key, *shader, &push)) {
        return;
    }

    if (!m_impl->spritePipeline || !m_impl->spritePipeline->handle()) {
        URON_WARN("drawSprite: spritePipeline no disponible");
        return;
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

    vkCmdPushConstants(cmd, m_impl->spritePipeline->layout(),
                       VK_SHADER_STAGE_VERTEX_BIT |
                       VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(SpritePush), &push);

    vkCmdDraw(cmd, 6, 1, 0, 0);
}

bool VulkanRenderer::drawSpriteCustom(VkCommandBuffer cmd, VulkanImage* img,
                                      u64 texKey, const Shader& shader,
                                      const void* spritePush) {
    auto* data = static_cast<ShaderData*>(shader.internal());
    if (!data) return false;

    // BUG-018: esta ShaderData tenia otra entrada (recarga => handle nuevo,
    // o destroy => handle 0). Retirarla a pendingDestroy y seguir.
    for (auto it = m_impl->customShaders.begin();
         it != m_impl->customShaders.end(); ) {
        if (it->second.owner == data && it->first != data->handle) {
            m_impl->pendingDestroy.push_back(std::move(it->second));
            it = m_impl->customShaders.erase(it);
        } else {
            ++it;
        }
    }

    if (!data->loaded) return false;

    Impl::CustomShader& entry = m_impl->customShaders[data->handle];
    entry.owner = data;
    if (!entry.attempted) {
        entry.attempted = true;
        auto* custom = new VulkanShader();
        if (custom->create(m_impl->context, m_impl->swapchain,
                           m_impl->renderPass, data->vertPath,
                           data->fragPath, data->desc)) {
            entry.shader = custom;
            URON_INFO("Pipeline custom creado: " + data->vertPath);
        } else {
            delete custom;
            URON_ERROR("Pipeline custom fallo (" + data->vertPath +
                       "); se usa el pipeline de sprites");
        }
    }
    if (!entry.shader) return false;

    VkDescriptorSet set = VK_NULL_HANDLE;
    auto itSet = entry.sets.find(texKey);
    if (itSet != entry.sets.end()) {
        set = itSet->second;
    } else {
        set = entry.shader->allocateSet(m_impl->context, img->view(),
                                        img->sampler());
        if (set == VK_NULL_HANDLE) return false;
        entry.sets[texKey] = set;
    }

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_impl->quadBuffer, &offset);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      entry.shader->pipeline());
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            entry.shader->layout(),
                            0, 1, &set, 0, nullptr);

    CustomPush push{};
    std::memcpy(&push, spritePush, SPRITE_PUSH_BYTES);
    data->packUniforms(&push, CUSTOM_PUSH_BYTES);

    vkCmdPushConstants(cmd, entry.shader->layout(),
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, CUSTOM_PUSH_BYTES, &push);

    vkCmdDraw(cmd, 6, 1, 0, 0);
    return true;
}

void VulkanRenderer::setViewProjection(const Mat4& viewProj) {
    m_impl->viewProj = viewProj;
}

// FNV-1a sobre los datos crudos: valida que la malla cacheada por puntero
// no haya cambiado de contenido entre frames (el puntero puede reciclarse).
static u64 meshSignature(const Mesh& mesh) {
    u64 h = 14695981039346656037ull;
    auto mix = [&h](const void* p, size_t n) {
        const auto* b = static_cast<const unsigned char*>(p);
        for (size_t i = 0; i < n; ++i) {
            h ^= b[i];
            h *= 1099511628211ull;
        }
    };
    auto& v = mesh.vertices();
    auto& idx = mesh.indices();
    mix(v.data(), v.size() * sizeof(Vertex));
    mix(idx.data(), idx.size() * sizeof(u32));
    return h;
}

static bool createGpuBuffer(Context& ctx, VkDeviceSize size,
                            VkBufferUsageFlags usage,
                            VkBuffer& buf, VkDeviceMemory& mem) {
    VkBufferCreateInfo bi{};
    bi.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size        = size;
    bi.usage       = usage;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(ctx.device(), &bi, nullptr, &buf) != VK_SUCCESS) {
        URON_ERROR("vkCreateBuffer (mesh) fallo");
        return false;
    }

    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(ctx.device(), buf, &req);

    VkMemoryAllocateInfo ai{};
    ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize  = req.size;
    ai.memoryTypeIndex = ctx.findMemoryType(req.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (ai.memoryTypeIndex == UINT32_MAX) {
        URON_ERROR("findMemoryType fallo (mesh)");
        vkDestroyBuffer(ctx.device(), buf, nullptr);
        buf = VK_NULL_HANDLE;
        return false;
    }

    if (vkAllocateMemory(ctx.device(), &ai, nullptr, &mem) != VK_SUCCESS) {
        URON_ERROR("vkAllocateMemory (mesh) fallo");
        vkDestroyBuffer(ctx.device(), buf, nullptr);
        buf = VK_NULL_HANDLE;
        return false;
    }

    vkBindBufferMemory(ctx.device(), buf, mem, 0);
    return true;
}

static void uploadToBuffer(Context& ctx, VkDeviceMemory mem,
                           const void* src, VkDeviceSize size) {
    void* data = nullptr;
    vkMapMemory(ctx.device(), mem, 0, size, 0, &data);
    std::memcpy(data, src, static_cast<size_t>(size));
    vkUnmapMemory(ctx.device(), mem);
}

void VulkanRenderer::drawMesh(const Mesh& mesh, const Mat4& transform) {
    if (!mesh.isValid() || mesh.indices().empty()) return;

    if (!m_impl->pipeline.handle()) {
        if (!m_impl->warnedMeshPipeline) {
            m_impl->warnedMeshPipeline = true;
            URON_WARN("drawMesh: pipeline de mallas no disponible");
        }
        return;
    }

    u64 sig = meshSignature(mesh);

    Impl::MeshGpu* gpu = nullptr;
    auto it = m_impl->meshCache.find(&mesh);
    if (it != m_impl->meshCache.end() && it->second.sig == sig) {
        gpu = &it->second;
    } else {
        Impl::MeshGpu g{};
        if (it != m_impl->meshCache.end()) {
            auto dev = m_impl->context.device();
            auto& old = it->second;
            if (old.vbuf) vkDestroyBuffer(dev, old.vbuf, nullptr);
            if (old.vmem) vkFreeMemory(dev, old.vmem, nullptr);
            if (old.ibuf) vkDestroyBuffer(dev, old.ibuf, nullptr);
            if (old.imem) vkFreeMemory(dev, old.imem, nullptr);
            g = it->second = Impl::MeshGpu{};
        }

        VkDeviceSize vsize = mesh.vertices().size() * sizeof(Vertex);
        VkDeviceSize isize = mesh.indices().size() * sizeof(u32);

        if (!createGpuBuffer(m_impl->context, vsize,
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                             g.vbuf, g.vmem) ||
            !createGpuBuffer(m_impl->context, isize,
                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                             g.ibuf, g.imem)) {
            if (g.vbuf) vkDestroyBuffer(m_impl->context.device(), g.vbuf, nullptr);
            if (g.vmem) vkFreeMemory(m_impl->context.device(), g.vmem, nullptr);
            return;
        }

        uploadToBuffer(m_impl->context, g.vmem,
                       mesh.vertices().data(), vsize);
        uploadToBuffer(m_impl->context, g.imem,
                       mesh.indices().data(), isize);

        g.indexCount = mesh.indexCount();
        g.sig        = sig;

        m_impl->meshCache[&mesh] = g;
        gpu = &m_impl->meshCache[&mesh];
    }

    MeshPush push{};
    Mat4 mvp = m_impl->viewProj * transform;
    std::memcpy(push.mvp, &mvp, sizeof(push.mvp));

    auto cmd = m_impl->commandBuffers[m_impl->currentFrame];
    VkDeviceSize offset = 0;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      m_impl->pipeline.handle());
    vkCmdBindVertexBuffers(cmd, 0, 1, &gpu->vbuf, &offset);
    vkCmdBindIndexBuffer(cmd, gpu->ibuf, 0, VK_INDEX_TYPE_UINT32);
    vkCmdPushConstants(cmd, m_impl->pipeline.layout(),
                       VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(MeshPush), &push);
    vkCmdDrawIndexed(cmd, gpu->indexCount, 1, 0, 0, 0);
}

void VulkanRenderer::waitIdle() {
    if (m_impl->context.device()) {
        vkDeviceWaitIdle(m_impl->context.device());
    }
}

u32 VulkanRenderer::width() const {
    // BUG-030: si la swapchain no existe/esta en 0, no devolvemos 0
    // (division por cero en quien calcule aspect).
    u32 w = m_impl->swapchain.extent().width;
    return w ? w : 1;
}

u32 VulkanRenderer::height() const {
    u32 h = m_impl->swapchain.extent().height;
    return h ? h : 1;
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