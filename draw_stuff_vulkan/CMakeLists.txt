# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)


# Add source to this project's executable.
add_executable (draw_stuff_vulkan "src/main_draw_stuff_vulkan.cpp" ${SPV_SHADERS})

add_dependencies(draw_stuff_vulkan shaders)

target_link_libraries(draw_stuff_vulkan PUBLIC 
	glfw 
	${CMAKE_DL_LIBS} 
	${GLFW_LIBRARIES}
	Vulkan::Vulkan
	nlohmann_json::nlohmann_json
	meshoptimizer
	MyLibraries
	)