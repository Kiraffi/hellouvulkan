# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

# Add source to this project's executable.
add_executable (font_render "src/main_font_renderer.cpp")



target_link_libraries(font_render PRIVATE MyGlad MyLibraries)
# TODO: Add tests and install targets if needed.
target_link_libraries(font_render PUBLIC 
	glfw
	OpenGL::GL 
	${CMAKE_DL_LIBS} 
	${SDL2_LIBRARY}
	#-lSDL2
	)