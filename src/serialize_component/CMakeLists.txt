# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#print_variables()


#include(${CMAKE_CURRENT_LIST_DIR}/testcmake2.cmake)

set(DEF_FILE "${CMAKE_CURRENT_LIST_DIR}/test.def")
set(DEF_OUT_FILE "${CMAKE_CURRENT_LIST_DIR}/generated_header.h")

# Run the command whenever building
add_custom_command(
    TARGET PRE_BUILD
    OUTPUT ${DEF_OUT_FILE}
    COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DEF_FILE=${DEF_FILE} -DDES_DEF_FILE=${DEF_OUT_FILE}
    -P ${CMAKE_CURRENT_LIST_DIR}/testcmake2.cmake
    DEPENDS ${DEF_FILE})

add_custom_target(GENERATED_HEADERS DEPENDS DEPENDS ${DEF_FILE})

# Add source to this project's executable.
add_executable (serialize_component
    "${DEF_OUT_FILE}"
    "src/serialize_component.cpp"
    "src/components.h"
    "src/components.cpp")

add_dependencies(serialize_component shaders GENERATED_HEADERS)

target_link_libraries(serialize_component PRIVATE
    MyLibraries
    #MyVulkan
    #MyImgui
    glfw
)
