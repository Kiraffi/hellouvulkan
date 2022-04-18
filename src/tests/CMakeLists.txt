# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)


# Add source to this project's executable.
add_executable (tests "main_test.cpp")

target_link_libraries(tests PRIVATE
	MyLibraries
	)