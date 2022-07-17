# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()

# Add source to this project's executable.
add_executable (sound_test "src/sound_test.cpp")

add_dependencies(sound_test shaders)

target_link_libraries(sound_test PRIVATE
    MyLibraries
    MyVulkan
    MyImgui
    MyGui
    glfw
    MyAudio
    )

#lto? need to figure out how to turn it on only for release, if wanting to use it
#set_property(TARGET sound_test PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
