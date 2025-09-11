cmake_minimum_required(VERSION 3.22)
project(img_dlib)

# find_package(dlib 19.24 REQUIRED)
add_library(dlib INTERFACE)
target_include_directories(dlib INTERFACE
	$<BUILD_INTERFACE:/mnt/d/cpp/libs/dlib>
	$<INSTALL_INTERFACE:include>
)

# add_subdirectory(/mnt/d/cpp/libs/dlib build)
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

add_executable(img_dlib "img_dlib.cc")
# target_link_libraries(img_dlib dlib:dlib)
target_link_libraries(img_dlib dlib)
