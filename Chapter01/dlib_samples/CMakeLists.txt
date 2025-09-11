cmake_minimum_required(VERSION 3.22)
project(dlib-samples)

# find_package(dlib 19.24 REQUIRED)
add_subdirectory(/mnt/d/cpp/libs/dlib build)

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

add_executable(linalg_dlib "linalg_dlib.cc")
target_link_libraries(linalg_dlib dlib::dlib)

add_executable(linreg_dlib "linreg_dlib.cc")
target_link_libraries(linreg_dlib dlib::dlib)

add_executable(img_dlib "img_dlib.cc")
target_link_libraries(img_dlib dlib::dlib)
