add_library(MyLibraries
	core/app.h
	core/app.cpp
	core/mytypes.h
	ogl/shaderbuffer.h
	ogl/shaderbuffer.cpp
	ogl/shader.cpp
	core/timer.h
	core/timer.cpp

#	core/vulkan_app.h
#	core/vulkan_app.cpp
#
#	myvulkan/vulkandevice.h
#	myvulkan/vulkandevice.cpp
#	myvulkan/vulkanresource.h
#	myvulkan/vulkanresource.cpp
#	myvulkan/vulkanshader.h
#	myvulkan/vulkanshader.cpp
#	myvulkan/vulkanswapchain.h
#	myvulkan/vulkanswapchain.cpp

	"math/matrix.h"
	"math/matrix.cpp"
	"math/plane.h"
	"math/quaternion.cpp"
	"math/quaternion.h"
	"math/vector3.cpp"
	"math/vector3.h"
	"math/general_math.h"
	 )

target_include_directories(MyLibraries PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/"	
	${CMAKE_DL_LIBS}
	#${SDL2_LIBRARY}
	#-lSDL2

	)