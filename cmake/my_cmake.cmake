#set(SHOW_VARIABLES true)
#if (SHOW_VARIABLES)
#endif (SHOW_VARIABLES)
#include(${CMAKE_HOME_DIRECTORY}/cmake/my_cmake.cmake)
#PROJECT_SOURCE_DIR

function(PRINT_VARIABLES)
    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()
