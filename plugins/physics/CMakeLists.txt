add_library(uron_plugin_physics STATIC
    src/Physics.cpp
)

add_library(Uron::Plugin::Physics ALIAS uron_plugin_physics)

target_include_directories(uron_plugin_physics
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(uron_plugin_physics
    PUBLIC
        Uron::Core
)

target_compile_features(uron_plugin_physics PUBLIC cxx_std_17)

set_target_properties(uron_plugin_physics PROPERTIES
    OUTPUT_NAME "uron_plugin_physics"
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
    target_compile_options(uron_plugin_physics PRIVATE /W4 /permissive-)
else()
    target_compile_options(uron_plugin_physics PRIVATE -Wall -Wextra -Wpedantic)
endif()