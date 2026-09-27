# ============================================================================
#  Uron Core - Libreria base del motor
#  Solo render + ventana + sistema de plugins + escenas + texturas.
#  Todo lo demas son plugins externos.
# ============================================================================

set(URON_CORE_SOURCES
    src/Logger.cpp
    src/Window.cpp
    src/Input.cpp
    src/Engine.cpp

    src/Plugin/PluginManager.cpp
    src/Plugin/PluginRegistry.cpp
    src/Plugin/PluginContext.cpp
    src/Plugin/ApiPlugin.cpp

    src/scene/Node2D.cpp
    src/scene/Scene2D.cpp
    src/scene/Sprite2D.cpp

    src/render/Texture.cpp
    src/render/Shader.cpp
    src/render/Mesh.cpp

    src/vulkan/VulkanContext.cpp
    src/vulkan/Swapchain.cpp
    src/vulkan/VulkanUtil.cpp
    src/vulkan/Pipeline.cpp
    src/vulkan/SpritePipeline.cpp
    src/vulkan/VulkanImage.cpp
    src/vulkan/VulkanShader.cpp
    src/vulkan/VulkanRenderer.cpp
)

set(URON_CORE_HEADERS
    include/Uron/Uron.h
    include/Uron/Types.h
    include/Uron/Logger.h
    include/Uron/Window.h
    include/Uron/Input.h
    include/Uron/Engine.h

    include/Uron/math/Math.h
    include/Uron/math/Vec2.h
    include/Uron/math/Vec3.h
    include/Uron/math/Vec4.h
    include/Uron/math/Mat3.h
    include/Uron/math/Mat4.h
    include/Uron/math/Quat.h

    include/Uron/render/Color.h
    include/Uron/render/Renderer.h
    include/Uron/render/Mesh.h
    include/Uron/render/Texture.h
    include/Uron/render/Shader.h
    include/Uron/render/Camera.h
    include/Uron/render/Transform.h
    include/Uron/render/Render.h

    include/Uron/scene/Node2D.h
    include/Uron/scene/Scene2D.h
    include/Uron/scene/Sprite2D.h
    include/Uron/scene/Camera2D.h

    include/Uron/Plugin/IPlugin.h
    include/Uron/Plugin/PluginID.h
    include/Uron/Plugin/PluginContext.h
    include/Uron/Plugin/PluginManager.h
    include/Uron/Plugin/PluginRegistry.h
    include/Uron/Plugin/PluginConfig.h
    include/Uron/Plugin/ApiPlugin.h
    include/Uron/Plugin/IPluginApi.h
    include/Uron/Plugin/ServiceRegistry.h
    include/Uron/Plugin/EventBus.h
    include/Uron/Plugin/Macros.h
)

set(URON_CORE_PRIVATE_HEADERS
    src/render/ShaderInternal.h
    src/vulkan/VulkanContext.h
    src/vulkan/Swapchain.h
    src/vulkan/VulkanUtil.h
    src/vulkan/Pipeline.h
    src/vulkan/SpritePipeline.h
    src/vulkan/VulkanImage.h
    src/vulkan/VulkanShader.h
    src/vulkan/VulkanRenderer.h
)

add_library(uron_core STATIC
    ${URON_CORE_SOURCES}
    ${URON_CORE_HEADERS}
    ${URON_CORE_PRIVATE_HEADERS}
)

add_library(Uron::Core ALIAS uron_core)

target_include_directories(uron_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(uron_core
    PUBLIC
        Vulkan::Vulkan
        glfw
)

target_compile_features(uron_core PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(uron_core PRIVATE /W4 /permissive-)
    target_compile_definitions(uron_core PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(uron_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(uron_core PROPERTIES
    OUTPUT_NAME "uron_core"
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

source_group("Headers"           FILES ${URON_CORE_HEADERS})
source_group("Sources"           FILES ${URON_CORE_SOURCES})
source_group("Sources\\Plugin"   FILES
    src/Plugin/PluginManager.cpp
    src/Plugin/PluginRegistry.cpp
    src/Plugin/PluginContext.cpp
    src/Plugin/ApiPlugin.cpp
)
source_group("Sources\\Scene"    FILES
    src/scene/Node2D.cpp
    src/scene/Scene2D.cpp
    src/scene/Sprite2D.cpp
)
source_group("Sources\\Render"   FILES
    src/render/Texture.cpp
    src/render/Shader.cpp
    src/render/Mesh.cpp
)
source_group("Sources\\Vulkan"   FILES
    src/vulkan/VulkanContext.cpp
    src/vulkan/Swapchain.cpp
    src/vulkan/VulkanUtil.cpp
    src/vulkan/Pipeline.cpp
    src/vulkan/SpritePipeline.cpp
    src/vulkan/VulkanImage.cpp
    src/vulkan/VulkanShader.cpp
    src/vulkan/VulkanRenderer.cpp
)

install(TARGETS uron_core
    EXPORT  UronTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/Uron
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)