cmake_minimum_required(VERSION 3.22)
project(arrayfire_samples)

# find_package(ArrayFire REQUIRED)
# add_library(arrayfire INTERFACE
# 	$<BUILD_INTERFACE:/mnt/d/cpp/libs/arrayfire/include>
# 	$<INSTALL_INTERFACE:include>
# )
add_subdirectory(/mnt/d/cpp/libs/arrayfire /mnt/d/cpp/libs/arrayfire/build)

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

add_executable(linalg_af "linalg_af.cc")

# target_include_directories(linalg_af
# 	PRIVATE
# 		/mnt/d/cpp/libs/arrayfire/include
# 		/mnt/d/cpp/libs/arrayfire/extern/half/include
# 		/mnt/d/cpp/libs/arrayfire/build/include
# )
target_link_libraries(linalg_af ArrayFire::afcpu)

add_executable(linreg_af "linreg_af.cc")
target_link_libraries(linreg_af ArrayFire::afcpu)
