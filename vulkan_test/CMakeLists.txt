# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

file( GLOB LIB_SOURCES src/*.cpp )
file( GLOB LIB_HEADERS src/*.h )
#file( GLOB LIB_SOURCES2 src/myvulkan/*.cpp )
#file( GLOB LIB_HEADERS2 src/myvulkan/*.h )

# Add source to this project's executable.
add_executable (vulkan_test ${LIB_SOURCES} ${LIB_HEADERS} ${LIB_SOURCES2} ${LIB_HEADERS2})

target_link_libraries(vulkan_test PRIVATE MyGlad MyLibraries)
# TODO: Add tests and install targets if needed.
target_link_libraries(vulkan_test PUBLIC 
	glfw 
	${CMAKE_DL_LIBS} 
	${GLFW_LIBRARIES} 
	Vulkan::Vulkan
	meshoptimizer	
)

