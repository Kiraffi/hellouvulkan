# Whenever this glob's value changes, cmake will rerun and update the build with the
# new/removed files.

project ("hellouvulkan libs" VERSION 0.0.1)

include("${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake")
#print_variables()

set(DEF_FILE "${CMAKE_CURRENT_LIST_DIR}/mylibscomponents.def")
set(DEF_OUT_FILE "${CMAKE_CURRENT_LIST_DIR}/components/generated")
set(DEF_CMAKE_FILE "${CMAKE_HOME_DIRECTORY}/cmake/generatecomponents.cmake")

# Run the command whenever building
add_custom_command(
    TARGET PRE_BUILD
    OUTPUT "${DEF_OUT_FILE}_components.h" "${DEF_OUT_FILE}_components.cpp" "${DEF_OUT_FILE}_systems.h" "${DEF_OUT_FILE}_systems.cpp"
    COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DEF_FILE=${DEF_FILE} -DDES_DEF_FILE=${DEF_OUT_FILE}
    -P ${DEF_CMAKE_FILE}
    DEPENDS ${DEF_FILE} ${DEF_CMAKE_FILE})

add_custom_target(MYLIBS_GENERATED_HEADERS DEPENDS DEPENDS ${DEF_FILE})

add_library(MyLibraries OBJECT EXCLUDE_FROM_ALL

    "app/glfw_keys.h"
    "app/inputapp.h"
    "app/inputapp.cpp"
    "app/vulkan_app.h"
    "app/vulkan_app.cpp"

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

    "components/components.h"
    "components/generated_components.h"
    "components/generated_systems.h"
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

    "components/components.cpp"
    "components/generated_components.cpp"
    "components/generated_systems.cpp"
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





    "myvulkan/myvma.cpp"
    "myvulkan/myvulkan.cpp"
    "myvulkan/vulkaninitparameters.h"
    "myvulkan/vulkaninitparameters.cpp"
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

add_library(MyImgui OBJECT EXCLUDE_FROM_ALL
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

add_dependencies(MyLibraries MYLIBS_GENERATED_HEADERS)

target_link_libraries(MyLibraries PUBLIC
    #glfw
    Vulkan::Vulkan
    ${WINMM_LIB}

    Imgui
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
