# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()

# Add source to this project's executable.
add_executable (font_draw_vulkan "src/main_font_draw_vulkan.cpp")

add_dependencies(font_draw_vulkan shaders)

target_link_libraries(font_draw_vulkan PRIVATE
    MyLibraries
    MyVulkan
    MyImgui
    )