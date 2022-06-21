# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()

# Add source to this project's executable.
add_executable (game "src/game.cpp")

add_dependencies(game shaders)

target_link_libraries(game PRIVATE
    MyLibraries
    MyVulkan
    MyImgui
    )