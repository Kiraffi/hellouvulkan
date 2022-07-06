# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()

# Add source to this project's executable.
add_executable (sound_test_pulse "src/sound_pulse.cpp")

target_link_libraries(sound_test_pulse PRIVATE
    MyLibraries
    -lpulse
    )