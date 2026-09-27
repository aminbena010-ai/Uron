cmake_minimum_required(VERSION 3.20)

project(Uron
    VERSION 0.3.0
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