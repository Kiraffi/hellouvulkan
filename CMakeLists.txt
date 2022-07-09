# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.15)
project ("hellouvulkan" VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)
#ADD_DEFINITIONS(-DCMAKE_GENERATOR_PLATFORM=x64)
#ADD_DEFINITIONS(-DCMAKE_GENERATOR_PLATFORM=x86)

#set(CMAKE_CXX_FLAGS_RELEASE "-O2")
#set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g /d1reportTime")

# adding multiprocessor compilation, in case generating visual studio solution separately
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
#add_compile_options($<$<CXX_COMPILER_ID:MSVC>-ftime-ftrace)

# imgui stuff to support vulkan with x86
ADD_DEFINITIONS(-DImTextureID=ImU64)

# miniaudio remove unneccessary stuff
ADD_DEFINITIONS(-DMA_NO_DECODING)
ADD_DEFINITIONS(-DMA_NO_ENCODING)
ADD_DEFINITIONS(-DMA_NO_WAV)

ADD_DEFINITIONS(-DMA_NO_FLAC)
ADD_DEFINITIONS(-DMA_NO_MP3)
ADD_DEFINITIONS(-DMA_NO_GENERATION)
#ADD_DEFINITIONS(-DMA_NO_SSE2)
ADD_DEFINITIONS(-DMA_NO_AVX2)
#ADD_DEFINITIONS(-DMA_NO_NEON)




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
set(SPIRV_BINARY_OUT_DIR ${SHADER_DIR}/spvs)

file(GLOB_RECURSE SPV_SHADERS CONFIGURE_DEPENDS ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag ${SHADER_DIR}/*.comp)

set(SPIRV_BINARY_FILES, "")
# helper function to generate custom commands for filename
function(COMPILE_SHADER SHADER_PERMUTATION)
    set(SPIRVLocal "${SPIRV_BINARY_OUT_DIR}/${SHADER_FILENAME}_${SHADER_PERMUTATION}.spv")
    set(SPIRV "${SPIRVLocal}" PARENT_SCOPE)
    message(STATUS "Compiling ${SHADER} into ${SPIRV}")
    add_custom_command(
        OUTPUT ${SPIRVLocal}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -DSPECIAL_PERMUTATION=${SHADER_PERMUTATION} --target-env=vulkan1.1 -o ${SPIRVLocal}
        DEPENDS ${SHADER} ${SHADER_DIR}/common.h)
endfunction()

foreach(SHADER IN LISTS SPV_SHADERS)
    get_filename_component(SHADER_FILENAME ${SHADER} NAME)
    get_filename_component(SHADER_FILENAME_DIR ${SHADER} DIRECTORY)

    set(VALUE_OUT 1)

    #Read shader file, and find line #define PERMUTATIONS number, if not we have VALUE_OUT 1
    #if found, evaluate string to number, which will be set to VALUE_OUT
    FILE(READ "${SHADER}" contents)
    STRING(REGEX REPLACE ".*#define[ \t]+(PERMUTATIONS[ \t]+[0-9]*).*" "\\1" permutations ${contents} )
    STRING(REGEX REPLACE "(PERMUTATIONS).*" "\\1" permutation ${permutations})
    if(permutation STREQUAL "PERMUTATIONS")
        STRING(REGEX REPLACE "PERMUTATIONS[ \t]([0-9]*).*" "\\1" amount ${permutations} )
        math(EXPR VALUE_OUT "${amount}" OUTPUT_FORMAT DECIMAL)

        message(STATUS "Permutations ${amount}")
    endif()

    while(VALUE_OUT GREATER 0)
        # Count down the VALUE_OUT, there is probably far better way to do for loop, but...
        math(EXPR VALUE_OUT "${VALUE_OUT} - 1" OUTPUT_FORMAT DECIMAL)

        # Add filename_index.spv, from 1 to filename first number.
        # For example if basic3d.vert has PEERMUTATIONS defined as 4, will create
        ## { basic3d.vert_0.spv, basic3d.vert_1.spv, basic3d.vert_2.spv, basic3d.vert_3.spv }
        # If there is no PERMUTATIONS defined in shader-file then we just make shadername_0.spv
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


include_directories("external/glfw/include/")
include_directories("external/imgui/")
include_directories("external/miniaudio/")
include_directories("external/stb/")
include_directories("src/mylibs")
include_directories("external/VulkanMemoryAllocator/include")


set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
add_library(Imgui
    "external/imgui/imgui.cpp"
    "external/imgui/imgui_draw.cpp"
    "external/imgui/imgui_tables.cpp"
    "external/imgui/imgui_widgets.cpp"
    "external/imgui/backends/imgui_impl_glfw.cpp"
    "external/imgui/backends/imgui_impl_vulkan.cpp"
)


add_library(Miniaudio
    "external/miniaudio/extras/miniaudio_split/miniaudio.c"
)

# add_subdirectory("external/meshoptimizer")
add_subdirectory("external/glfw")
add_subdirectory("src")

#print_variables()
