cmake_minimum_required(VERSION 3.22)
project(blaze_samples)

# find_package(blaze 3.8.2 REQUIRED)
add_library(xtensor INTERFACE)
target_include_directories(xtensor INTERFACE
	$<BUILD_INTERFACE:/mnt/d/cpp/libs/xtensor/include>
	$<INSTALL_INTERFACE:include>
)

add_library(blas INTERFACE)
target_include_directories(blas INTERFACE
	$<BUILD_INTERFACE:/mnt/d/cpp/libs/xtensor-blas/include>
	$<INSTALL_INTERFACE:include>
)

add_library(xtl INTERFACE)
target_include_directories(xtl INTERFACE
	$<BUILD_INTERFACE:/mnt/d/cpp/libs/xtl/include>
	$<INSTALL_INTERFACE:include>
)

add_library(blaze INTERFACE)
target_include_directories(blaze INTERFACE
	$<BUILD_INTERFACE:/mnt/d/cpp/libs/blaze>
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

add_executable(linalg_blaze "linalg_blaze.cc")
target_link_libraries(linalg_blaze blas lapack blaze -fopenmp)

add_executable(linreg_blaze "linreg_blaze.cc")
target_link_libraries(linreg_blaze blas lapack blaze -fopenmp)
