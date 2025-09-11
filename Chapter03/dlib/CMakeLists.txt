cmake_minimum_required(VERSION 3.10)
project(dlib-sample)

# find_package(dlib 19.24 REQUIRED)
add_library(dlib INTERFACE)
target_include_directories(dlib INTERFACE
	$<BUILD_INTERFACE:/mnt/d/cpp/libs/dlib>
	$<INSTALL_INTERFACE:include>
)

set(PLOTCPP_PATH "" CACHE PATH "path to plotcpp install dir")

if (NOT PLOTCPP_PATH)
	message(FATAL_ERROR "Missing plotcpp include path, please specify PLOTCPP_PATH")
else()
	message("plotcpp path is ${PLOTCPP_PATH}")
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
	-Wall -Wextra -msse3 -fopenmp
	$<$<CONFIG:RELEASE>:-Ofast>
	$<$<CONFIG:DEBUG>:-O0>
	$<$<CONFIG:DEBUG>:-ggdb3>
)

add_compile_definitions(
	$<$<CONFIG:RELEASE>:NDEBUG>
)

include_directories(${PLOTCPP_PATH})

add_executable(grid-dlib "grid-dlib.cc")
# target_link_libraries(grid-dlib dlib::dlib)
target_link_libraries(grid-dlib dlib)
