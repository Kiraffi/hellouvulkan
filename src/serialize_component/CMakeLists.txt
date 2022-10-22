# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()


include(${CMAKE_CURRENT_LIST_DIR}/testcmake2.cmake)

# Add source to this project's executable.
add_executable (serialize_component "src/serialize_component.cpp" "src/components_macro.h" "src/components.h" "src/components.cpp")

add_dependencies(serialize_component shaders)

target_link_libraries(serialize_component PRIVATE
    MyLibraries
    #MyVulkan
    #MyImgui
    glfw
)
