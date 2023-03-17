# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.

cmake_minimum_required (VERSION 3.15)


project ("hellouvulkan Common-project" VERSION 0.0.1)

include("${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake")
#print_variables()

add_library(Common_Project OBJECT
        "common_project/common_project.cpp"
        "common_project/common_project.h")

target_link_libraries(Common_Project PUBLIC
    )