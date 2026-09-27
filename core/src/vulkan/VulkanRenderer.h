#pragma once
#include <Uron/render/Renderer.h>
#include <Uron/render/Color.h>
#include <vulkan/vulkan.h>

namespace Uron {
class Window;
class Texture;
}

namespace Uron::Vulkan {

class SpritePipeline;
class VulkanImage;

class VulkanRenderer : public Renderer {
public:
    VulkanRenderer();
    ~VulkanRenderer() override;

    bool init(Window* window) override;
    void shutdown() override;

    void beginFrame() override;
    void clear(const Color& c) override;
    void endFrame() override;

    void drawMesh(const Mesh&, const Mat4&) override {}
    void drawSprite(const Texture&, const Mat4&) override;
    void waitIdle() override;

    void setViewport(u32, u32, u32, u32) override {}
    void setClearColor(const Color& c) override { clear(c); }

    u32 width()  const override;
    u32 height() const override;

    VulkanImage* createImage(u32 width, u32 height, const u8* pixels);

private:
    bool createRenderPass();
    bool createFramebuffers();
    bool createCommandPool();
    bool createCommandBuffers();
    bool createSyncObjects();
    bool createQuadBuffer();

    void shutdownVulkan();

    struct Impl;
    Impl* m_impl;
};

}