# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)


# Add source to this project's executable.
add_executable (font_render_vulkan "src/main_font_renderer_vulkan.cpp")

add_dependencies(font_render_vulkan shaders)

target_link_libraries(font_render_vulkan PUBLIC 
	glfw 
	${CMAKE_DL_LIBS} 
	${GLFW_LIBRARIES} 
	Vulkan::Vulkan
	meshoptimizer
	MyLibraries
	)