cmake_minimum_required(VERSION 3.10)
project(mlpack-anomaly)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(mlpack 4.0.1 REQUIRED)

find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)

add_library(armadillo INTERFACE)
target_include_directories(armadillo INTERFACE
	$<BUILD_INTERFACE:/mnt/d/cpp/libs/armadillo-code/include>
	$<INSTALL_INTERFACE:include>
)

add_library(cereal INTERFACE)
target_include_directories(cereal INTERFACE
	$<BUILD_INTERFACE:/mnt/d/cpp/libs/cereal/include>
	$<INSTALL_INTERFACE:include>
)

add_library(ensmallen INTERFACE)
target_include_directories(ensmallen INTERFACE
	$<BUILD_INTERFACE:/mnt/d/cpp/libs/ensmallen/include>
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
	$<$<CONFIG:DEBUG>:--ggdb3>
)

add_link_options(-fopenmp)

add_compile_definitions(
	$<$<CONFIG:RELEASE>:NDEBUG>
)

include_directories(${PLOTCPP_PATH})
include_directories(${MLPACK_INCLUDE_DIR})

add_executable(mlpack-anomaly "mlpack-anomaly.cc")
target_link_directories(mlpack-anomaly PRIVATE ${CMAKE_PREFIX_PATH}/lib)
target_link_libraries(mlpack-anomaly ${MLPACK_LIBRARIES} armadillo cereal ensmallen ${BLAS_LIBRARIES} ${LAPACK_LIBRARIES})
