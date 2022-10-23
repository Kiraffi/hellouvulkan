# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()


include(${CMAKE_CURRENT_LIST_DIR}/testcmake2.cmake)

set(DEF_FILE "${CMAKE_CURRENT_LIST_DIR}/test.def")
set(DEF_OUT_FILE "${CMAKE_CURRENT_LIST_DIR}/generated_header.h")

#file(GLOB_RECURSE DEF_FILE CONFIGURE_DEPENDS "*.def")
#message(STATUS "Def file: ${DEF_FILE}")
PARSE_DEF_FILE("${DEF_FILE}" "${DEF_OUT_FILE}")

# Add source to this project's executable.
add_executable (serialize_component
    "generated_header.h"
    "src/serialize_component.cpp"
    "src/components.h"
    "src/components.cpp")

add_dependencies(shaders serialize_component)

target_link_libraries(serialize_component PRIVATE
    MyLibraries
    #MyVulkan
    #MyImgui
    glfw
)
