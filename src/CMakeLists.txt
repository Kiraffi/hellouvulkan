# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.15)

add_subdirectory ("mylibs")
add_subdirectory ("font_draw_vulkan")
add_subdirectory ("font_render_vulkan")
add_subdirectory ("space_shooter_vulkan")
add_subdirectory ("vulkan_test")
add_subdirectory ("draw_stuff_vulkan")
