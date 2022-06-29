# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()

# Add source to this project's executable.
add_executable (editor_test "src/editor_test.cpp")

add_dependencies(editor_test shaders)

target_link_libraries(editor_test PRIVATE
    MyLibraries
    MyVulkan
    MyImgui
    glfw
    )