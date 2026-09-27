# ============================================================================
#  Uron Examples
# ============================================================================

# ----------------------------------------------------------------------------
# Datos comunes (shaders + assets) -> bin/
# Un SOLO target evita la carrera de varios POST_BUILD copiando la misma
# carpeta en paralelo (cmake --build -j).
# ----------------------------------------------------------------------------
add_custom_target(uron_example_data ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_BINARY_DIR}/bin/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders
            ${CMAKE_BINARY_DIR}/bin/shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_BINARY_DIR}/bin/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/examples/hello_sprite/assets
            ${CMAKE_BINARY_DIR}/bin/assets
    COMMENT "Copiando shaders y assets a bin/"
)

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

add_dependencies(hello_uron uron_example_data)

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

add_dependencies(hello_physics uron_example_data)

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

add_dependencies(hello_scene uron_example_data)

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

add_dependencies(hello_sprite uron_example_data)
