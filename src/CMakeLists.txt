# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.15)

project ("hellouvulkan" VERSION 0.0.1)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()

add_subdirectory ("mylibs")
add_subdirectory ("font_draw_vulkan")
add_subdirectory ("font_render_vulkan")
add_subdirectory ("tests")
add_subdirectory ("space_shooter_vulkan")
#add_subdirectory ("vulkan_test")
add_subdirectory ("draw_stuff_vulkan")
add_subdirectory ("compute_test_vulkan")
add_subdirectory ("imgui_test")
add_subdirectory ("editor_test")
add_subdirectory ("game")
add_subdirectory ("sound_test")
add_subdirectory ("serialize_component")

#if (UNIX)
#    add_subdirectory ("sound_test_pulse")
#else()
#    add_subdirectory ("sound_test_dsound")
#endif()
