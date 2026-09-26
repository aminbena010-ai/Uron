add_executable(hello_scene
    hello_scene/main.cpp
)

target_link_libraries(hello_scene PRIVATE
    Uron::Core
)

set_target_properties(hello_scene PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)