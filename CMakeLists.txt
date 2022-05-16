# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.15)
project ("hellouvulkan" VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)
ADD_DEFINITIONS(-DCMAKE_GENERATOR_PLATFORM=x64)

include(${PROJECT_SOURCE_DIR}/cmake/my_cmake.cmake)
#set(OpenGL_GL_PREFERENCE "GLVND")
#set(OpenGL_GL_PREFERENCE LEGACY)

if (UNIX)
    #do something
endif (UNIX)

#find_package(OpenGL REQUIRED)
#find_package(Vulkan REQUIRED)

if (DEFINED VULKAN_SDK_PATH)
  set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include")
  set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib")
  set(Vulkan_FOUND "True")
else()
  find_package(Vulkan REQUIRED)
  message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
endif()
if (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
else()
	message(STATUS "Using vulkan lib at: ${Vulkan_LIBRARIES}")
endif()

#include_directories(${Vulkan_INCLUDE_DIR})
include_directories(${Vulkan_INCLUDE_DIRS})



set(GLSLC_EXECUTABLE glslc)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets/shader/vulkan_new)

#set(SPV_SHADERS
#    ${SHADER_DIR}/texturedquad.vert ${SHADER_DIR}/texturedquad.frag
#    #${SHADER_DIR}/*.comp
#)
file(GLOB_RECURSE SPV_SHADERS CONFIGURE_DEPENDS ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag ${SHADER_DIR}/*.comp
    #${SHADER_DIR}/*.geom ${SHADER_DIR}/*.tesc ${SHADER_DIR}/*.tese ${SHADER_DIR}/*.mesh ${SHADER_DIR}/*.task ${SHADER_DIR}/*.rgen ${SHADER_DIR}/*.rchit ${SHADER_DIR}/*.rmiss
    )

foreach(SHADER IN LISTS SPV_SHADERS)
    get_filename_component(FILENAME ${SHADER} NAME)
    message(STATUS "Compiling ${SHADER}")
    execute_process(COMMAND ${GLSLC_EXECUTABLE} ${SHADER} --target-env=vulkan1.3 -o ${SHADER_DIR}/${FILENAME}.spv)
endForeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})



set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" )
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" )
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" )

#ADD_DEFINITIONS(-DSHOW_INFO_MESSAGES=0)
#ADD_DEFINITIONS(-DVSYNC=2)
#ADD_DEFINITIONS(-DDISCRETE_GPU=1)
#ADD_DEFINITIONS(-DUSE_RENDERDOC_MARKERS=1)
#ADD_DEFINITIONS(-DUSE_GPU_DEBUG_VALIDATION=1)

include_directories("external/")
include_directories("external/glfw/include/")
include_directories("src/mylibs")
include_directories("external/VulkanMemoryAllocator/include")

#add_subdirectory("external/meshoptimizer")
add_subdirectory("external/glfw")
add_subdirectory("src")

#print_variables()



