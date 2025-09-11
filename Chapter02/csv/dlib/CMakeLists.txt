cmake_minimum_required(VERSION 3.22)
project(csv_dlib)

# add_subdirectory(/mnt/d/cpp/libs/dlib build)
add_library(dlib INTERFACE)
target_include_directories(dlib INTERFACE
	$<BUILD_INTERFACE:/mnt/d/cpp/libs/dlib>
	$<INSTALL_INTERFACE:include>
)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_VERBOSE_MAKEFILE ON)

add_compile_options(
	-Wall -Wextra -msse3 -fopenmp
	$<$<CONFIG:RELEASE>:-Ofast>
	$<$<CONFIG:DEBUG>:-O0>
	$<$<CONFIG:DEBUG>:-ggdb3>
)

add_compile_definitions(
	$<$<CONFIG:RELEASE>:NDEBUG>
)

add_executable(csv_dlib "csv_dlib.cc")
target_link_libraries(csv_dlib dlib)
