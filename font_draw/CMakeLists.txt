# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

# Add source to this project's executable.
add_executable (font_draw "src/main_font_draw.cpp")

target_link_libraries(font_draw PRIVATE MyGlad MyLibraries)
# TODO: Add tests and install targets if needed.
target_link_libraries(font_draw 
	PUBLIC OpenGL::GL 
	${CMAKE_DL_LIBS} 
#	${SDL2_LIBRARY}
#	-lSDL2
	glfw
	#SDL2::SDL2
    #SDL2::SDL2main
)

