cmake_minimum_required(VERSION 3.22)
project(csv_sample)

set(CSV_LIB_PATH "" CACHE PATH "Path to csv library include dir")

if (NOT CSV_LIB_PATH)
	message(FATAL_ERROR "Missing CSV lib install path, please specify CSV_LIB_PATH")
else()
	message("CSV lib path is ${CSV_LIB_PATH}")
endif()

# find_package(Eigen3 3.4.0 REQUIRED)
add_library(Eigen INTERFACE)
target_include_directories(Eigen INTERFACE
	$<BUILD_INTERFACE:/mnt/d/cpp/libs/eigen>
	$<INSTALL_INTERFACE:include>
)

add_library(csv INTERFACE)
target_include_directories(csv INTERFACE
	$<BUILD_INTERFACE:/mnt/d/cpp/libs/fast-cpp-csv-parser>
	$<INSTALL_INTERFACE:include>
)

find_package(Threads REQUIRED)

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

include_directories(${CSV_LIB_PATH})

add_executable(csv_sample "csv.cc")
# target_link_libraries (csv_sample Eigen3::Eigen)
target_link_libraries(csv_sample Eigen csv Threads::Threads)
