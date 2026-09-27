cmake_minimum_required(VERSION 3.20)

project(Uron
    VERSION 0.4.0
    DESCRIPTION "Uron Engine - Ligero, flexible, letal."
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

if(NOT TARGET Vulkan::Vulkan)
    message(FATAL_ERROR "Vulkan no encontrado")
endif()

option(URON_BUILD_EXAMPLES "Build Uron examples" ON)
option(URON_BUILD_PLUGINS  "Build Uron official plugins" ON)
option(URON_BUILD_TESTS    "Build Uron tests" OFF)

add_subdirectory(core)

if(URON_BUILD_PLUGINS)
    add_subdirectory(plugins)
endif()

if(URON_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

message(STATUS "")
message(STATUS "===========================================")
message(STATUS "  Uron Engine ${PROJECT_VERSION}")
message(STATUS "  Build type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Examples:     ${URON_BUILD_EXAMPLES}")
message(STATUS "  Plugins:      ${URON_BUILD_PLUGINS}")
message(STATUS "===========================================")
message(STATUS "")

# ----------------------------------------------------------------------------
# Shaders: glslc compila *.vert / *.frag a bin/shaders/*.spv en cada build.
# (los .spv NO estan en git, asi que un clone nuevo los genera solo)
# ----------------------------------------------------------------------------
find_program(GLSLC glslc)
if(GLSLC)
    file(GLOB URON_SHADER_SOURCES
        ${CMAKE_SOURCE_DIR}/shaders/*.vert
        ${CMAKE_SOURCE_DIR}/shaders/*.frag
    )
    set(URON_SHADER_OUTPUTS "")
    foreach(src ${URON_SHADER_SOURCES})
        get_filename_component(fname ${src} NAME)
        set(out ${CMAKE_BINARY_DIR}/bin/shaders/${fname}.spv)
        add_custom_command(
            OUTPUT  ${out}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/shaders
            COMMAND ${GLSLC} ${src} -o ${out}
            DEPENDS ${src}
            COMMENT "glslc ${fname}"
            VERBATIM
        )
        list(APPEND URON_SHADER_OUTPUTS ${out})
    endforeach()

    if(URON_SHADER_OUTPUTS)
        add_custom_target(uron_shaders ALL DEPENDS ${URON_SHADER_OUTPUTS})
        message(STATUS "  Shaders: glslc -> bin/shaders/*.spv")
    endif()
else()
    message(WARNING "glslc no encontrado: ejecuta shaders/compile.sh a mano")
endif()

if(TARGET uron_shaders AND TARGET uron_example_data)
    add_dependencies(uron_example_data uron_shaders)
endif()
