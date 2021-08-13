cmake_minimum_required (VERSION 3.15)


target_include_directories(MyLibraries PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/"
	glfw-3.3.4/include/
	json/include/
	)