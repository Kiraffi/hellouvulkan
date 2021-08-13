# Whenever this glob's value changes, cmake will rerun and update the build with the
# new/removed files.
#file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")

add_library(MyLibraries
	"core/mytypes.h"
	"core/general.h"
	"core/general.cpp"
	"core/timer.h"
	"core/timer.cpp"


	"core/vulkan_app.h"
	"core/vulkan_app.cpp"
	
	
	"myvulkan/vulkandevice.h"
	"myvulkan/vulkandevice.cpp"
	"myvulkan/vulkanhelperfuncs.h"
	"myvulkan/vulkanmemorymanager.cpp"
	"myvulkan/vulkanmemorymanager.h"
	"myvulkan/vulkanresource.cpp"
	"myvulkan/vulkanshader.h"
	"myvulkan/vulkanshader.cpp"
	"myvulkan/vulkanswapchain.cpp"

	"math/matrix.cpp"
	"math/quaternion.cpp"
	"math/vector3.cpp"

	"render/font_render.cpp"
	"render/font_render.h"
	 "myvulkan/vulkanmemorymanager.cpp"
	"myvulkan/vulkanmemorymanager.h"
	
	"core/json.cpp"
	"core/json.h"
	)

target_include_directories(MyLibraries PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/"	
	"${CMAKE_DL_LIBS}"
	"Vulkan::Vulkan"
	"glfw"
	"math"
	"myvulkan"
	"render"
	#"math/vector3.h"
	#"math/general_math.h"
	#"math/quaternion.h"
	#"math/matrix.h"
	#"math/plane.h"
	#"myvulkan/vulkanswapchain.h"
	#"myvulkan/vulkanresource.h"

	#"render/font_render.h"

	${VULKAN_INCLUDE_DIR}
	${VULKAN_INCLUDE_DIRS}
	)