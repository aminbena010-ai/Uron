#include "VulkanContext.h"
#include <Uron/Logger.h>
#include <GLFW/glfw3.h>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <stdexcept>
#include <algorithm>

namespace Uron::Vulkan {

static const std::vector<const char*> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static const char* kValidationLayer = "VK_LAYER_KHRONOS_validation";

// Validacion: en Debug (NDEBUG no definido) se intenta activar siempre que
// la capa exista; URON_VALIDATION=0 la fuerza a OFF y =1 a ON (tambien en
// Release, util para pruebas).
static bool validationRequested() {
    const char* env = std::getenv("URON_VALIDATION");
    if (env && env[0] == '0') return false;
    if (env && env[0] == '1') return true;
#ifndef NDEBUG
    return true;
#else
    return false;
#endif
}

static bool validationLayerAvailable() {
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());
    for (const auto& l : layers) {
        if (std::strcmp(l.layerName, kValidationLayer) == 0) return true;
    }
    return false;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void*) {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        URON_ERROR(std::string("[Vulkan] ") + (data ? data->pMessage : ""));
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        URON_WARN(std::string("[Vulkan] ") + (data ? data->pMessage : ""));
    }
    return VK_FALSE;
}

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
    if (m_debugMessenger) {
        auto fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_instance,
                                  "vkDestroyDebugUtilsMessengerEXT"));
        if (fn) fn(m_instance, m_debugMessenger, nullptr);
        m_debugMessenger = VK_NULL_HANDLE;
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

    std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);

    // BUG-041: capa de validacion si se pidio y esta instalada.
    std::vector<const char*> layers;
    if (validationRequested() && validationLayerAvailable()) {
        layers.push_back(kValidationLayer);

        uint32_t extCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> availExt(extCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extCount,
                                               availExt.data());
        for (const auto& e : availExt) {
            if (std::strcmp(e.extensionName,
                            VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                break;
            }
        }
        URON_INFO(std::string("Validacion Vulkan activa (") +
                  kValidationLayer + ")");
    }

    uint32_t availCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availCount, nullptr);
    std::vector<VkExtensionProperties> avail(availCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availCount, avail.data());

    VkInstanceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &appInfo;
    ci.enabledLayerCount   = static_cast<uint32_t>(layers.size());
    ci.ppEnabledLayerNames = layers.data();

    for (const auto& e : avail) {
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        if (std::strcmp(e.extensionName,
                        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
            extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            ci.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
            break;
        }
#endif
    }

    ci.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    ci.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&ci, nullptr, &m_instance) != VK_SUCCESS) {
        URON_ERROR("vkCreateInstance fallo");
        return false;
    }

    if (!layers.empty()) {
        auto fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_instance,
                                  "vkCreateDebugUtilsMessengerEXT"));
        if (fn) {
            VkDebugUtilsMessengerCreateInfoEXT dci{};
            dci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            dci.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            dci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            dci.pfnUserCallback = debugCallback;
            fn(m_instance, &dci, nullptr, &m_debugMessenger);
        }
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
        uint32_t g = 0, p = 0;
        if (isDeviceSuitable(dev, g, p)) {
            m_physicalDevice = dev;
            m_graphicsFamily = g;
            m_presentFamily  = p;
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

bool Context::isDeviceSuitable(VkPhysicalDevice dev, uint32_t& outGraphics,
                               uint32_t& outPresent) {
    uint32_t qCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, nullptr);
    std::vector<VkQueueFamilyProperties> queues(qCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, queues.data());

    bool hasGraphics = false;
    bool hasPresent  = false;

    for (uint32_t i = 0; i < qCount; ++i) {
        if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            outGraphics = i;
            hasGraphics = true;
        }
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, m_surface, &present);
        if (present) {
            outPresent = i;
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

    VkPhysicalDeviceFeatures supported{};
    vkGetPhysicalDeviceFeatures(m_physicalDevice, &supported);

    VkPhysicalDeviceFeatures features{};
    features.fillModeNonSolid = supported.fillModeNonSolid;
    features.samplerAnisotropy = supported.samplerAnisotropy;

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
    return UINT32_MAX;
}

}