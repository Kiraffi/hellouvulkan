# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)


#add_executable (space_shooter "src/main_space_shooter.cpp")
add_executable (vulkan_test 
	"src/main_vulkan_test.cpp"

	"src/camera.cpp"
	"src/camera.h"
	"src/fontsystem.cpp"
	"src/fontsystem.h"
	"src/keyhandler.cpp"
	"src/keyhandler.h"
	"src/model.cpp"
	"src/model.h"
	"src/transform.h"
)


#file( GLOB LIB_SOURCES src/*.cpp )
#file( GLOB LIB_HEADERS src/*.h )
#file( GLOB LIB_SOURCES2 src/myvulkan/*.cpp )
#file( GLOB LIB_HEADERS2 src/myvulkan/*.h )

# Add source to this project's executable.
#add_executable (vulkan_test ${LIB_SOURCES} ${LIB_HEADERS})

#target_link_libraries(vulkan_test PRIVATE MyGlad MyLibraries)
# TODO: Add tests and install targets if needed.
target_link_libraries(vulkan_test PUBLIC 
	glfw 
	${CMAKE_DL_LIBS} 
	${GLFW_LIBRARIES} 
	Vulkan::Vulkan
	meshoptimizer
	MyGlad
	MyLibraries
	#${VULKAN_INCLUDE_DIR}
	#${VULKAN_INCLUDE_DIRS}
)

