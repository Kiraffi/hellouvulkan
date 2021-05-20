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



#if (MSVC)
#	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
#
#	set(SDL2_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/external/sdl2/include")
#
#	# Support both 32 and 64 bit builds
#	if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
#		set(SDL2_LIBRARY "${CMAKE_CURRENT_LIST_DIR}/external/sdl2/lib/x64/SDL2.lib;${CMAKE_CURRENT_LIST_DIR}/external/sdl2/lib/x64/SDL2main.lib")
#	else ()
#		set(SDL2_LIBRARY "${CMAKE_CURRENT_LIST_DIR}/external/sdl2/lib/x86/SDL2.lib;${CMAKE_CURRENT_LIST_DIR}/external/sdl2/lib/x86/SDL2main.lib")
#	endif ()
#	string(STRIP "${SDL2_LIBRARY}" SDL2_LIBRARY)
#endif (MSVC)


if (UNIX)
    #do something
endif (UNIX)


#find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)
#include_directories(${SDL2_INCLUDE_DIRS})

#add_subdirectory("externallibs")
add_subdirectory("external/glfw-3.3.4")
add_subdirectory("external/meshoptimizer")
add_subdirectory("mylibs")
add_subdirectory("external")
add_subdirectory ("font_draw")
add_subdirectory ("font_render")
add_subdirectory ("space_shooter")
add_subdirectory ("vulkan_test")

