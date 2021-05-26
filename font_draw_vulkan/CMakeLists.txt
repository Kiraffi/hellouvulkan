# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

# Add source to this project's executable.
add_executable (font_draw_vulkan "src/main_font_draw_vulkan.cpp")


add_dependencies(font_draw_vulkan shaders)
set_target_properties(
    font_draw_vulkan PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

target_link_libraries(font_draw_vulkan PRIVATE MyGlad MyLibraries)
# TODO: Add tests and install targets if needed.
target_link_libraries(font_draw_vulkan PUBLIC 
	glfw 
	${CMAKE_DL_LIBS} 
	${GLFW_LIBRARIES} 
	Vulkan::Vulkan
	meshoptimizer
	MyGlad
	MyLibraries
	)