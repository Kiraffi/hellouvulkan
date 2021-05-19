add_library(MyLibraries
	core/app.h
	core/timer.h
	core/mytypes.h
	ogl/shaderbuffer.h

	core/app.cpp
	core/timer.cpp
	ogl/shaderbuffer.cpp
	ogl/shader.cpp
	)

target_include_directories(MyLibraries PUBLIC 
	glfw 
	"${CMAKE_CURRENT_SOURCE_DIR}/"	
	${CMAKE_DL_LIBS} 
	${SDL2_LIBRARY}
	-lSDL2
	)