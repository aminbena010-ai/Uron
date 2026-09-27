#include "VulkanImage.h"
#include "VulkanContext.h"
#include <Uron/Logger.h>
#include <cstring>

namespace Uron::Vulkan {

bool VulkanImage::create(Context& ctx,
                         uint32_t width,
                         uint32_t height,
                         const uint8_t* pixels,
                         uint32_t channels) {
    m_device = ctx.device();   // BUG-034: destroy() no depende del ctx pasado
    m_width  = width;
    m_height = height;

    if (channels != 4) {
        URON_ERROR("VulkanImage solo soporta RGBA8 (channels=4), recibió " +
                   std::to_string(channels));
        return false;
    }

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) *
                             static_cast<VkDeviceSize>(height) * 4;

    VkBuffer       stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo bi{};
    bi.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size        = imageSize;
    bi.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(ctx.device(), &bi, nullptr, &stagingBuffer) != VK_SUCCESS) {
        URON_ERROR("vkCreateBuffer (staging) fallo");
        return false;
    }

    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(ctx.device(), stagingBuffer, &req);

    VkMemoryAllocateInfo ai{};
    ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize  = req.size;
    ai.memoryTypeIndex = ctx.findMemoryType(req.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (ai.memoryTypeIndex == UINT32_MAX) {
        vkDestroyBuffer(ctx.device(), stagingBuffer, nullptr);
        URON_ERROR("findMemoryType fallo (staging)");
        return false;
    }

    if (vkAllocateMemory(ctx.device(), &ai, nullptr, &stagingMemory) != VK_SUCCESS) {
        vkDestroyBuffer(ctx.device(), stagingBuffer, nullptr);
        URON_ERROR("vkAllocateMemory (staging) fallo");
        return false;
    }

    vkBindBufferMemory(ctx.device(), stagingBuffer, stagingMemory, 0);

    void* data = nullptr;
    vkMapMemory(ctx.device(), stagingMemory, 0, imageSize, 0, &data);
    std::memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(ctx.device(), stagingMemory);

    VkImageCreateInfo ici{};
    ici.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType     = VK_IMAGE_TYPE_2D;
    ici.extent.width  = width;
    ici.extent.height = height;
    ici.extent.depth  = 1;
    ici.mipLevels     = 1;
    ici.arrayLayers   = 1;
    ici.format        = VK_FORMAT_R8G8B8A8_SRGB;
    ici.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ici.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                        VK_IMAGE_USAGE_SAMPLED_BIT;
    ici.samples       = VK_SAMPLE_COUNT_1_BIT;
    ici.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(ctx.device(), &ici, nullptr, &m_image) != VK_SUCCESS) {
        vkDestroyBuffer(ctx.device(), stagingBuffer, nullptr);
        vkFreeMemory(ctx.device(), stagingMemory, nullptr);
        URON_ERROR("vkCreateImage fallo");
        return false;
    }

    VkMemoryRequirements imgReq{};
    vkGetImageMemoryRequirements(ctx.device(), m_image, &imgReq);

    VkMemoryAllocateInfo imgAi{};
    imgAi.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    imgAi.allocationSize  = imgReq.size;
    imgAi.memoryTypeIndex = ctx.findMemoryType(imgReq.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (imgAi.memoryTypeIndex == UINT32_MAX) {
        vkDestroyImage(ctx.device(), m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        vkDestroyBuffer(ctx.device(), stagingBuffer, nullptr);
        vkFreeMemory(ctx.device(), stagingMemory, nullptr);
        URON_ERROR("findMemoryType fallo (imagen)");
        return false;
    }

    if (vkAllocateMemory(ctx.device(), &imgAi, nullptr, &m_memory) != VK_SUCCESS) {
        vkDestroyImage(ctx.device(), m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        vkDestroyBuffer(ctx.device(), stagingBuffer, nullptr);
        vkFreeMemory(ctx.device(), stagingMemory, nullptr);
        URON_ERROR("vkAllocateMemory (imagen) fallo");
        return false;
    }
    vkBindImageMemory(ctx.device(), m_image, m_memory, 0);

    VkCommandPoolCreateInfo pci{};
    pci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pci.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    pci.queueFamilyIndex = ctx.graphicsFamily();

    VkCommandPool pool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(ctx.device(), &pci, nullptr, &pool) != VK_SUCCESS) {
        vkDestroyBuffer(ctx.device(), stagingBuffer, nullptr);
        vkFreeMemory(ctx.device(), stagingMemory, nullptr);
        URON_ERROR("vkCreateCommandPool (upload) fallo");
        return false;
    }

    VkCommandBufferAllocateInfo cbai{};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = pool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(ctx.device(), &cbai, &cmd);

    VkCommandBufferBeginInfo cbi{};
    cbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &cbi);

    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = m_image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    barrier.oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    vkCmdCopyBufferToImage(cmd, stagingBuffer, m_image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo uploadSubmit{};
    uploadSubmit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    uploadSubmit.commandBufferCount = 1;
    uploadSubmit.pCommandBuffers    = &cmd;
    vkQueueSubmit(ctx.graphicsQueue(), 1, &uploadSubmit, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx.graphicsQueue());

    vkDestroyCommandPool(ctx.device(), pool, nullptr);
    vkDestroyBuffer(ctx.device(), stagingBuffer, nullptr);
    vkFreeMemory(ctx.device(), stagingMemory, nullptr);

    VkImageViewCreateInfo vi{};
    vi.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image    = m_image;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format   = VK_FORMAT_R8G8B8A8_SRGB;
    vi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vi.subresourceRange.levelCount = 1;
    vi.subresourceRange.layerCount = 1;

    vkCreateImageView(ctx.device(), &vi, nullptr, &m_view);

    VkSamplerCreateInfo si{};
    si.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    si.magFilter    = VK_FILTER_LINEAR;
    si.minFilter    = VK_FILTER_LINEAR;
    si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    si.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    vkCreateSampler(ctx.device(), &si, nullptr, &m_sampler);

    URON_INFO("VulkanImage creada: " +
              std::to_string(width) + "x" + std::to_string(height));

    return true;
}

void VulkanImage::destroy(Context& ctx) {
    // BUG-034: usa el dispositivo guardado en create(); ctx solo es
    // fallback si create() nunca se ejecuto.
    VkDevice dev = m_device ? m_device : ctx.device();
    if (m_sampler) { vkDestroySampler(dev, m_sampler, nullptr); m_sampler = VK_NULL_HANDLE; }
    if (m_view)    { vkDestroyImageView(dev, m_view, nullptr);  m_view    = VK_NULL_HANDLE; }
    if (m_image)   { vkDestroyImage(dev, m_image, nullptr);     m_image   = VK_NULL_HANDLE; }
    if (m_memory)  { vkFreeMemory(dev, m_memory, nullptr);      m_memory  = VK_NULL_HANDLE; }
    m_device = VK_NULL_HANDLE;
}

}