cmake_minimum_required(VERSION 3.22)
project(xtensor_samples)

# find_package(xtensor 0.24.5 REQUIRED)
# find_package(xtensor-blas 0.20.0 REQUIRED)
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

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_VERBOSE_MAKEFILE ON)

add_compile_options(
	-Wall -Wextra -msse3
	$<$<CONFIG:RELEASE>:-Ofast>
	$<$<CONFIG:DEBUG>:-O0>
	$<$<CONFIG:DEBUG>:-ggdb3>
)

add_compile_definitions(
	$<$<CONFIG:RELEASE>:NDEBUG>
)

add_executable(linalg_xtensor "linalg_xtensor.cc")
# target_include_directories(linalg_xtensor PRIVATE ${xtensor_INCLUDE_DIRS})
target_include_directories(linalg_xtensor PRIVATE ../../libs/${xtensor_INCLUDE_DIRS})
target_link_libraries(linalg_xtensor xtensor blas xtl)
