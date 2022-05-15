# Whenever this glob's value changes, cmake will rerun and update the build with the
# new/removed files.

project ("hellouvulkan libs" VERSION 0.0.1)
 

add_library(MyLibraries

    "container/bytebuffer.cpp"
    "container/mymemory.cpp"

    "core/camera.cpp"
    "core/general.cpp"
    "core/timer.cpp"
    "core/json.cpp"
    "core/vulkan_app.cpp"

    "math/matrix.cpp"
    "math/quaternion.cpp"
    "math/vector3.cpp"

    "myvulkan/myvulkan.cpp"
    "myvulkan/uniformbuffermanager.cpp"
    "myvulkan/vulkanresources.cpp"

    "render/font_render.cpp"
    "model/gltf.h"
    "model/gltf.cpp"
)

target_include_directories(MyLibraries PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/"
    )


target_link_libraries(MyLibraries PUBLIC
    glfw   
    Vulkan::Vulkan
    winMM.Lib
    #meshoptimizer
)