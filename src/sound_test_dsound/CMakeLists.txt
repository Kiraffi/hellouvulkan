# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()

# Add source to this project's executable.
add_executable (sound_test_dsound "src/sound_test_dsound.cpp")

add_dependencies(sound_test_dsound shaders)

target_link_libraries(sound_test_dsound PRIVATE
    MyLibraries
    MyVulkan
    MyImgui
    glfw
    Miniaudio
    )