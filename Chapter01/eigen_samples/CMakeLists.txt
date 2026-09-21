cmake_minimum_required(VERSION 3.22)
project(eigen_samples)

# find_package(Eigen3 3.4.0 REQUIRED)
add_library(Eigen INTERFACE)
target_include_directories(Eigen INTERFACE
	$<BUILD_INTERFACE:/home/MIND_SUPPORT/Documents/libs/eigen>
	$<INSTALL_INTERFACE:include>
)
# list(APPEND CMAKE_PREFIX_PATH "/home/MIND_SUPPORT/Documents/libs/eigen")
# 
# find_package(Eigen 3.4 REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
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

add_executable(linalg_eigen "linalg_eigen.cc")
# Link target Eigen3::Eigen chính thức do find_package tạo ra
target_link_libraries(linalg_eigen Eigen)

add_executable(linreg_eigen "linreg_eigen.cc")
target_link_libraries(linreg_eigen Eigen)
