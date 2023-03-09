# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()

# Add source to this project's executable.
add_executable (common_project "src/common_project.cpp.cpp" "src/common_project.h")

add_dependencies(common_project shaders)

target_link_libraries(common_project PRIVATE
    MyLibraries
    Vulkan::Vulkan
    Imgui
    #MyVulkan
    #MyImgui
    glfw
    )