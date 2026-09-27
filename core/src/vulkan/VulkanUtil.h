#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace Uron::Vulkan::Util {

// Lectura de un .spv y creacion del modulo de shader. Antes estaba
// duplicado (identico) en Pipeline.cpp, SpritePipeline.cpp y
// VulkanShader.cpp; una sola copia aqui (BUG-021).
std::vector<char> readFile(const char* path);

VkShaderModule createModule(VkDevice dev, const std::vector<char>& code);

}
