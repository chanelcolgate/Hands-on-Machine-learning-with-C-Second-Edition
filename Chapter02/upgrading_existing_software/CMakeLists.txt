cmake_minimum_required(VERSION 3.22)
project(polymorph_sample)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_VERBOSE_MAKEFILE ON)

add_compile_options(
	-Wall -Wextra
	$<$<CONFIG:RELEASE>:-Ofast>
	$<$<CONFIG:DEUBG>:-O0>
)

add_compile_definitions(
	$<$<CONFIG:RELEASE>:NDEBUG>
)

add_executable(datalgos "datalgos.cc")
add_executable(override "override.cc")
add_executable(overloadcoin "overloadcoin.cc")
add_executable(genericfunc "genericfunc.cc")

add_executable(gendatastruct "gendatastruct.cc")
# target_include_directories(gendatastruct PUBLIC .)
