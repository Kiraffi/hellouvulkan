# Whenever this glob's value changes, cmake will rerun and update the build with the
# new/removed files.

project ("hellouvulkan libs" VERSION 0.0.1)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()


add_library(MyLibraries OBJECT EXCLUDE_FROM_ALL

    "container/bytebuffer.h"
    "container/mymemory.h"
    "container/podvector.h"
    "container/podvectorsbase.h"
    "container/podvectortype.h"
    "container/podvectortypedefine.h"
    "container/string.h"
    "container/stringview.h"
    "container/vector.h"
    "container/vectorbase.h"
    "container/vectorsbase.h"

    "components/transform.h"

    "core/camera.h"
    "core/file.h"
    "core/general.h"
    "core/timer.h"
    "core/json.h"
    "core/writejson.h"

    "math/matrix.h"
    "math/ray.h"
    "math/quaternion.h"
    "math/vector3.h"

    "model/gltf.h"

    "myvulkan/uniformbuffermanager.h"

    "scene/gameentity.h"
    "scene/scene.h"

    "model/animation.h"
    "resources/globalresources.h"




    "container/bytebuffer.cpp"
    "container/podvectortype.cpp"
    "container/string.cpp"
    "container/mymemory.cpp"
    "container/stringview.cpp"

    "components/transform.cpp"

    "core/camera.cpp"
    "core/image.cpp"
    "core/file.cpp"
    "core/general.cpp"
    "core/timer.cpp"
    "core/json.cpp"
    "core/writejson.cpp"

    "math/matrix.cpp"
    "math/ray.cpp"
    "math/quaternion.cpp"
    "math/vector3.cpp"

    "model/gltf.cpp"

    "myvulkan/uniformbuffermanager.cpp"

    "scene/gameentity.cpp"
    "scene/scene.cpp"

    "model/animation.cpp"
    "resources/globalresources.cpp"









    "core/vulkan_app.h"

    "myvulkan/myvulkan.h"
    "myvulkan/vulkanresources.h"
    "myvulkan/shader.h"

    "render/convertrendertarget.h"
    "render/font_render.h"
    "render/lightrendersystem.h"
    "render/linerendersystem.h"
    "render/meshrendersystem.h"
    "render/tonemaprendersystem.h"

    "render/lightingrendertargets.h"
    "render/meshrendertargets.h"





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

)

#add_library(MyAudio OBJECT EXCLUDE_FROM_ALL
#    "audio/myaudio.h"

#    "audio/myaudio.cpp"

#)

add_library(MyGui OBJECT EXCLUDE_FROM_ALL
    "mygui/mygui.h"


    "mygui/mygui.cpp"
)

add_library(MyImgui EXCLUDE_FROM_ALL
    "gui/componentviews.h"

    "render/myimguirenderer.h"

    "gui/guiutil.h"
    "gui/editorsystem.h"



    "gui/transformview.cpp"
    "gui/componentviews.cpp"

    "render/myimguirenderer.cpp"

    "gui/guiutil.cpp"
    "gui/editorsystem.cpp"
)

#add_library(MyVulkan OBJECT EXCLUDE_FROM_ALL
#)


if (WIN32)
    set(WINMM_LIB winMM.Lib)
else()
    set(WINMM_LIB  "")
endif()


target_link_libraries(MyLibraries PUBLIC
    glfw
    Vulkan::Vulkan
    ${WINMM_LIB}
    #meshoptimizer
    #PRIVATE MyLibraries

)


#target_link_libraries(MyAudio PUBLIC
#    Miniaudio
#)

target_link_libraries(MyGui PUBLIC
)


target_link_libraries(MyImgui PUBLIC
    Imgui
)

#target_link_libraries(MyVulkan PUBLIC
#)
