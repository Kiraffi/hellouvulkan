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


# Shader compileing
set(GLSLC_EXECUTABLE glslc)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets/shader/vulkan_new)

file(GLOB_RECURSE SPV_SHADERS CONFIGURE_DEPENDS ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag ${SHADER_DIR}/*.comp)

set(SPIRV_BINARY_FILES, "")

# helper function to generate custom commands for filename
function(COMPILE_SHADER SHADER_PERMUTATION)
    set(SPIRV "${SHADER}_${SHADER_PERMUTATION}.spv" PARENT_SCOPE)
    set(SPIRV "${SHADER}_${SHADER_PERMUTATION}.spv")
    message(STATUS "Compiling ${SHADER} into ${SPIRV}")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -DSPECIAL_PERMUTATION=${SHADER_PERMUTATION} --target-env=vulkan1.1 -o ${SPIRV}
        DEPENDS ${SHADER} ${SHADER_DIR}"/common.h")
endfunction()

foreach(SHADER IN LISTS SPV_SHADERS)
    get_filename_component(FILENAME ${SHADER} NAME)

    # probably the most horrible way to get first number from string?
    string(REGEX REPLACE "[A-Za-z0-9_]+[_]([0-9]+).[A-Za-z]+" "\\1" FIRST_LETTER ${FILENAME} )
    set(VALUE_OUT 1)
    if(FIRST_LETTER STRLESS_EQUAL "999999999999999" AND NOT FILENAME STREQUAL FIRST_LETTER)
        #parse the string number into number.
        #message(STATUS "number ${FIRST_LETTER}")
        math(EXPR VALUE_OUT "${FIRST_LETTER}" OUTPUT_FORMAT DECIMAL)
    endif()
    while(VALUE_OUT GREATER 0)


        # Count down the VALUE_OUT, there is probably far better way to do for loop, but...
        math(EXPR VALUE_OUT "${VALUE_OUT} - 1" OUTPUT_FORMAT DECIMAL)

        # Add filename_index.spv, from 1 to filename first number.
        # For example 3shader.comp -> { 3shader.comp_0.spv, 3shader.comp_1.spv, 3shader.comp_2.spv }
        # If there is no number then we just make shadername_1.spv
        compile_shader("${VALUE_OUT}")
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    endwhile()
endForeach()

add_custom_target(shaders DEPENDS ${SPIRV_BINARY_FILES})



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
