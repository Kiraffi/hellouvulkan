add_library(MyLibraries
	core/app.h
	core/app.cpp
	core/mytypes.h
	ogl/shaderbuffer.h
	ogl/shaderbuffer.cpp
	ogl/shader.cpp
	core/timer.h
	core/timer.cpp

	core/vulkan_app.h
	core/vulkan_app.cpp

	myvulkan/vulkandevice.h
	myvulkan/vulkandevice.cpp
	myvulkan/vulkanresource.cpp
	myvulkan/vulkanshader.h
	myvulkan/vulkanshader.cpp
	myvulkan/vulkanswapchain.cpp

	"math/matrix.cpp"
	"math/quaternion.cpp"
	"math/vector3.cpp"
	 )

target_include_directories(MyLibraries PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/"	
	${CMAKE_DL_LIBS}
	#${SDL2_LIBRARY}
	#-lSDL2
	Vulkan::Vulkan
	glfw
	"math/vector3.h"
	"math/general_math.h"
	"math/quaternion.h"
	"math/matrix.h"
	"math/plane.h"
	"myvulkan/vulkanswapchain.h"
	"myvulkan/vulkanresource.h"

	${VULKAN_INCLUDE_DIR}
	${VULKAN_INCLUDE_DIRS}
	"C:/VulkanSDK/1.2.176.1/Include"
	)