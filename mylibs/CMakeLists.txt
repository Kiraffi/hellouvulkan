add_library(MyLibraries
	core/app.cpp
	core/app.h
	core/mytypes.h
	ogl/shader.cpp
	ogl/shaderbuffer.cpp
	ogl/shaderbuffer.h
	)

target_include_directories(MyLibraries PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")