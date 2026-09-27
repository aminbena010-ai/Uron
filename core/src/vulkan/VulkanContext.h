#pragma once
#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace Uron::Vulkan {

class Context {
public:
    bool init(GLFWwindow* window);
    void shutdown();

    VkInstance       instance()       const { return m_instance; }
    VkSurfaceKHR     surface()        const { return m_surface; }
    VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    VkDevice         device()         const { return m_device; }
    VkQueue          graphicsQueue()  const { return m_graphicsQueue; }
    VkQueue          presentQueue()   const { return m_presentQueue; }
    uint32_t         graphicsFamily() const { return m_graphicsFamily; }
    uint32_t         presentFamily()  const { return m_presentFamily; }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) const;

private:
    bool createInstance();
    bool createSurface();
    bool pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice dev, uint32_t& outGraphics,
                          uint32_t& outPresent);
    bool createLogicalDevice();

    GLFWwindow*      m_window         = nullptr;
    VkInstance       m_instance       = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR     m_surface        = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice         m_device         = VK_NULL_HANDLE;
    VkQueue          m_graphicsQueue  = VK_NULL_HANDLE;
    VkQueue          m_presentQueue   = VK_NULL_HANDLE;
    uint32_t         m_graphicsFamily = 0;
    uint32_t         m_presentFamily  = 0;
    VkPhysicalDeviceProperties m_props{};
};

}