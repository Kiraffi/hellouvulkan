# Whenever this glob's value changes, cmake will rerun and update the build with the
# new/removed files.

project ("hellouvulkan libs" VERSION 0.0.1)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()


add_library(MyLibraries

    "container/bytebuffer.cpp"
    "container/mymemory.cpp"

    "components/transform.cpp"

    "core/camera.cpp"
    "core/file.cpp"
    "core/general.cpp"
    "core/timer.cpp"
    "core/json.cpp"
    "core/writejson.cpp"


    "math/matrix.cpp"
    "math/ray.cpp"
    "math/quaternion.cpp"
    "math/vector3.cpp"

    "math/matrix.h"
    "math/ray.h"
    "math/quaternion.h"
    "math/vector3.h"

    "model/gltf.cpp"

    "myvulkan/uniformbuffermanager.cpp"

    "scene/gameentity.cpp"
    "scene/scene.cpp"
)

add_library(MyImgui
    "gui/transformview.cpp"
    "gui/componentviews.h"
    "gui/gameentityview.cpp"
    "gui/componentviews.cpp"

    "render/myimgui.cpp"
)

add_library(MyVulkan
    "core/vulkan_app.cpp"

    "myvulkan/myvma.cpp"
    "myvulkan/myvulkan.cpp"
    "myvulkan/vulkanresources.cpp"
    "myvulkan/shader.cpp"


    "render/convertrendertarget.cpp"
    "render/font_render.cpp"
    "render/lightrendersystem.cpp"
    "render/linerendersystem.cpp"
    "render/meshrendersystem.cpp"
    "render/tonemaprendersystem.cpp"

    "render/lightingrendertargets.cpp"
    "render/meshrendertargets.cpp"
    "render/meshrendertargets.h"
    "render/lightingrendertargets.h"

)



target_include_directories(MyLibraries PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/"
    )




target_link_libraries(MyLibraries PUBLIC
)

target_include_directories(MyImgui PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/"
)


target_link_libraries(MyImgui PUBLIC
    Imgui
)

target_include_directories(MyVulkan PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/"
)

if (WIN32)
    set(WINMM_LIB winMM.Lib)
else()
    set(WINMM_LIB  "")
endif()

target_link_libraries(MyVulkan PUBLIC
    glfw
    Vulkan::Vulkan
    ${WINMM_LIB}
    #meshoptimizer
    PRIVATE MyLibraries
)
