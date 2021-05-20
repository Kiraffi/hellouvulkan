cmake_minimum_required (VERSION 3.15)

add_library(MyGlad
	glad/glad.h
	glad/glad.c
	glad/khrplatform.h

	tiny_obj_loader.h
	stb/stb_image.h

	
	)




target_include_directories(MyLibraries PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/"
	glfw-3.3.4/include/

	)