# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()

# Add source to this project's executable.
add_executable (imgui_test "src/imgui_test.cpp" ${IMGUI_DIR}/imgui_demo.cpp)

add_dependencies(imgui_test shaders)

target_link_libraries(imgui_test PRIVATE
    MyLibraries
    )