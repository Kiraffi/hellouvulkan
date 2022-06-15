# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()

# Add source to this project's executable.
add_executable (draw_stuff_vulkan "src/main_draw_stuff_vulkan.cpp")

add_dependencies(draw_stuff_vulkan shaders)

target_link_libraries(draw_stuff_vulkan PRIVATE
    MyLibraries
    MyVulkan
    MyImgui
    )