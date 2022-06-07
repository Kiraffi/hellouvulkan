# Whenever this glob's value changes, cmake will rerun and update the build with the
# new/removed files.

project ("hellouvulkan libs" VERSION 0.0.1)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()

add_library(MyLibraries

    "container/bytebuffer.cpp"
    "container/mymemory.cpp"

    "core/camera.cpp"
    "core/general.cpp"
    "core/timer.cpp"
    "core/json.cpp"
    "core/vulkan_app.cpp"

    "gui/examplegui.h"
    "gui/examplegui.cpp"

    "math/matrix.cpp"
    "math/quaternion.cpp"
    "math/vector3.cpp"

    "model/gltf.cpp"


    "myvulkan/myvma.cpp"
    "myvulkan/myvulkan.cpp"
    "myvulkan/uniformbuffermanager.cpp"
    "myvulkan/vulkanresources.cpp"
    "myvulkan/shader.cpp"

    "render/convertrendertarget.cpp"
    "render/font_render.cpp"
    "render/lightrendersystem.cpp"
    "render/linerendersystem.cpp"
    "render/meshrendersystem.cpp"
    "render/myimgui.cpp"
    "render/tonemaprendersystem.cpp"

    "render/lightingrendertargets.cpp"
    "render/meshrendertargets.cpp"
    "render/meshrendertargets.h"
    "render/lightingrendertargets.h"


    "scene/scene.cpp"

 "math/ray.h" "math/ray.cpp" "math/sphere.h" "math/bounds.h")

target_include_directories(MyLibraries PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/"
    )

if (WIN32)
    set(WINMM_LIB winMM.Lib)
else()
    set(WINMM_LIB  "")
endif()

target_link_libraries(MyLibraries PUBLIC
    glfw
    Vulkan::Vulkan
    ${WINMM_LIB}
    Imgui
    #meshoptimizer
)

