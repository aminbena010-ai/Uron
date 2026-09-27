#include "VulkanContext.h"
#include <Uron/Logger.h>
#include <GLFW/glfw3.h>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <algorithm>

namespace Uron::Vulkan {

static const std::vector<const char*> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

bool Context::init(GLFWwindow* window) {
    m_window = window;
    if (!createInstance())       return false;
    if (!createSurface())        return false;
    if (!pickPhysicalDevice())   return false;
    if (!createLogicalDevice())  return false;
    return true;
}

void Context::shutdown() {
    if (m_device) {
        vkDeviceWaitIdle(m_device);
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }
    if (m_surface) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
    if (m_instance) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
}

bool Context::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Uron";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName        = "Uron";
    appInfo.engineVersion      = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_2;

    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);

    VkInstanceCreateInfo ci{};
    ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo        = &appInfo;
    ci.enabledExtensionCount   = glfwExtCount;
    ci.ppEnabledExtensionNames = glfwExts;

    if (vkCreateInstance(&ci, nullptr, &m_instance) != VK_SUCCESS) {
        URON_ERROR("vkCreateInstance fallo");
        return false;
    }
    return true;
}

bool Context::createSurface() {
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
        URON_ERROR("glfwCreateWindowSurface fallo");
        return false;
    }
    return true;
}

bool Context::pickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
    if (count == 0) {
        URON_ERROR("No hay GPUs con Vulkan");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

    for (auto dev : devices) {
        if (isDeviceSuitable(dev)) {
            m_physicalDevice = dev;
            break;
        }
    }

    if (!m_physicalDevice) {
        URON_ERROR("No se encontro GPU adecuada");
        return false;
    }

    vkGetPhysicalDeviceProperties(m_physicalDevice, &m_props);
    URON_INFO(std::string("GPU: ") + m_props.deviceName);
    return true;
}

bool Context::isDeviceSuitable(VkPhysicalDevice dev) {
    uint32_t qCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, nullptr);
    std::vector<VkQueueFamilyProperties> queues(qCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, queues.data());

    bool hasGraphics = false;
    bool hasPresent  = false;

    for (uint32_t i = 0; i < qCount; ++i) {
        if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            m_graphicsFamily = i;
            hasGraphics = true;
        }
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, m_surface, &present);
        if (present) {
            m_presentFamily = i;
            hasPresent = true;
        }
        if (hasGraphics && hasPresent) break;
    }
    return hasGraphics && hasPresent;
}

bool Context::createLogicalDevice() {
    float priority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    std::vector<uint32_t> uniqueFamilies = { m_graphicsFamily, m_presentFamily };
    uniqueFamilies.erase(
        std::unique(uniqueFamilies.begin(), uniqueFamilies.end()),
        uniqueFamilies.end());

    for (uint32_t fam : uniqueFamilies) {
        VkDeviceQueueCreateInfo qi{};
        qi.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi.queueFamilyIndex = fam;
        qi.queueCount       = 1;
        qi.pQueuePriorities = &priority;
        queueInfos.push_back(qi);
    }

    VkPhysicalDeviceFeatures features{};

    VkDeviceCreateInfo ci{};
    ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.queueCreateInfoCount    = static_cast<uint32_t>(queueInfos.size());
    ci.pQueueCreateInfos       = queueInfos.data();
    ci.pEnabledFeatures        = &features;
    ci.enabledExtensionCount   = static_cast<uint32_t>(kDeviceExtensions.size());
    ci.ppEnabledExtensionNames = kDeviceExtensions.data();

    if (vkCreateDevice(m_physicalDevice, &ci, nullptr, &m_device) != VK_SUCCESS) {
        URON_ERROR("vkCreateDevice fallo");
        return false;
    }

    vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_presentFamily,  0, &m_presentQueue);
    return true;
}

uint32_t Context::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) const {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    return 0;
}

}