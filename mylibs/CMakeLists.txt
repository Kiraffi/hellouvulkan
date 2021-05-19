add_library(MyLibraries
	core/app.h
	core/timer.h
	core/mytypes.h
	ogl/shaderbuffer.h
	core/vulkan_app.h

	core/vulkan_app.cpp
	core/app.cpp
	core/timer.cpp
	ogl/shaderbuffer.cpp
	ogl/shader.cpp

	myvulkan/vulkandevice.h
	myvulkan/vulkandevice.cpp
	myvulkan/vulkanresource.h
	myvulkan/vulkanresource.cpp
	myvulkan/vulkanshader.h
	myvulkan/vulkanshader.cpp
	myvulkan/vulkanswapchain.h
	myvulkan/vulkanswapchain.cpp

	)

target_include_directories(MyLibraries PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/"	
	${CMAKE_DL_LIBS} 
	${SDL2_LIBRARY}
	-lSDL2

	)