# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.15)

project ("hellouvulkan" VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

set(OpenGL_GL_PREFERENCE "GLVND")
#set(OpenGL_GL_PREFERENCE LEGACY)

if (UNIX)
    #do something
endif (UNIX)

find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIR})
include_directories(${Vulkan_INCLUDE_DIRS})



set(GLSLC_EXECUTABLE glslc)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets/shader/vulkan_new)

#set(SPV_SHADERS 
#	${SHADER_DIR}/texturedquad.vert ${SHADER_DIR}/texturedquad.frag 
#	#${SHADER_DIR}/*.comp
#)
file(GLOB_RECURSE SPV_SHADERS CONFIGURE_DEPENDS ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag ${SHADER_DIR}/*.comp
    #${SHADER_DIR}/*.geom ${SHADER_DIR}/*.tesc ${SHADER_DIR}/*.tese ${SHADER_DIR}/*.mesh ${SHADER_DIR}/*.task ${SHADER_DIR}/*.rgen ${SHADER_DIR}/*.rchit ${SHADER_DIR}/*.rmiss
    )

find_package(Vulkan)

foreach(SHADER IN LISTS SPV_SHADERS)
    get_filename_component(FILENAME ${SHADER} NAME)
	message(STATUS "Compiling ${SHADER}")
	execute_process(COMMAND ${GLSLC_EXECUTABLE} ${SHADER} --target-env=vulkan1.1 -o ${SHADER_DIR}/${FILENAME}.spv)
endForeach() 
 
add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})


#set(SHOW_VARIABLES true)
if (SHOW_VARIABLES)
get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
endif (SHOW_VARIABLES)


set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")

#add_subdirectory("externallibs")
add_subdirectory("external/glfw-3.3.4")
add_subdirectory("external/meshoptimizer")
add_subdirectory("mylibs")
add_subdirectory("external")
add_subdirectory ("font_draw")
add_subdirectory ("font_render")
add_subdirectory ("space_shooter")
add_subdirectory ("font_draw_vulkan")
add_subdirectory ("font_render_vulkan")
add_subdirectory ("vulkan_test")


