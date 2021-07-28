# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)


# Add source to this project's executable.
add_executable (font_draw_vulkan "src/main_font_draw_vulkan.cpp" ${SPV_SHADERS})

add_dependencies(font_draw_vulkan shaders)

target_link_libraries(font_draw_vulkan PUBLIC 
	glfw 
	${CMAKE_DL_LIBS} 
	${GLFW_LIBRARIES}
	MyGlad
	MyLibraries
	Vulkan::Vulkan
	meshoptimizer
	MyGlad
	MyLibraries
	)