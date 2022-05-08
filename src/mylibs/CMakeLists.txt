# Whenever this glob's value changes, cmake will rerun and update the build with the
# new/removed files.

project ("hellouvulkan libs" VERSION 0.0.1)
 

add_library(MyLibraries
    "core/general.cpp"
    "core/timer.cpp"
    "core/camera.cpp"

    "core/vulkan_app.cpp"

    "container/bytebuffer.cpp"
    "container/mymemory.cpp"

    "myvulkan/myvulkan.cpp"

    #"myvulkan/vulkandevice.cpp"
    #"myvulkan/vulkanmemorymanager.cpp"
    #"myvulkan/vulkanresource.cpp"
    #"myvulkan/vulkanshader.cpp"
    #"myvulkan/vulkanswapchain.cpp"

    "math/matrix.cpp"
    "math/quaternion.cpp"
    "math/vector3.cpp"

    "render/font_render.cpp"
    #"myvulkan/vulkanmemorymanager.cpp"

    #"core/json.cpp"


     "myvulkan/vulkaninitparameters.h")
target_include_directories(MyLibraries PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/"
    )

#add_library(MyLibraries
#    "core/mytypes.h"
#    "core/general.h"
#    "core/timer.h"
#
#
#    "core/vulkan_app.h"
#
#
#    "myvulkan/vulkandevice.h"
#    "myvulkan/vulkanhelperfuncs.h"
#    "myvulkan/vulkanmemorymanager.h"
#    "myvulkan/vulkanshader.h"
#
#
#    "render/font_render.h"
#    "myvulkan/vulkanmemorymanager.h"
#
#    "core/json.h"
#    )
#
#target_include_directories(MyLibraries PUBLIC
#    "${CMAKE_CURRENT_SOURCE_DIR}/"
#    #"${CMAKE_DL_LIBS}"
#    #"Vulkan::Vulkan"
#    #"glfw"
#    #"math"
#    #"myvulkan"
#    #"render"
#    #"math/vector3.h"
#    #"math/general_math.h"
#    #"math/quaternion.h"
#    #"math/matrix.h"
#    #"math/plane.h"
#    #"myvulkan/vulkanswapchain.h"
#    #"myvulkan/vulkanresource.h"
#
#    #"render/font_render.h"
#
#    #${VULKAN_INCLUDE_DIR}
#    #${VULKAN_INCLUDE_DIRS}
#    )