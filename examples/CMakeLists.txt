# ============================================================================
#  Uron Examples
# ============================================================================

# ----------------------------------------------------------------------------
# hello_uron - Ejemplo minimo: ventana + clear
# ----------------------------------------------------------------------------
add_executable(hello_uron
    hello_uron/main.cpp
)

target_link_libraries(hello_uron PRIVATE
    Uron::Core
)

set_target_properties(hello_uron PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET hello_uron POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:hello_uron>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:hello_uron>/shaders
)

# ----------------------------------------------------------------------------
# hello_physics - Ejemplo del plugin Physics
# ----------------------------------------------------------------------------
add_executable(hello_physics
    hello_physics/main.cpp
)

target_link_libraries(hello_physics PRIVATE
    Uron::Core
    Uron::Plugin::Physics
)

set_target_properties(hello_physics PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET hello_physics POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:hello_physics>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:hello_physics>/shaders
)

# ----------------------------------------------------------------------------
# hello_scene - Ejemplo de escenas y nodos 2D
# ----------------------------------------------------------------------------
add_executable(hello_scene
    hello_scene/main.cpp
)

target_link_libraries(hello_scene PRIVATE
    Uron::Core
)

set_target_properties(hello_scene PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET hello_scene POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:hello_scene>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:hello_scene>/shaders
)

# ----------------------------------------------------------------------------
# hello_sprite - Ejemplo de sprites con textura
# ----------------------------------------------------------------------------
add_executable(hello_sprite
    hello_sprite/main.cpp
)

target_link_libraries(hello_sprite PRIVATE
    Uron::Core
)

set_target_properties(hello_sprite PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET hello_sprite POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:hello_sprite>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:hello_sprite>/shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:hello_sprite>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/examples/hello_sprite/assets
            $<TARGET_FILE_DIR:hello_sprite>/assets
)