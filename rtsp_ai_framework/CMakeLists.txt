cmake_minimum_required(VERSION 3.20)
project(rtsp_ai_framework LANGUAGES CXX)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libs: ${OpenCV_LIBS}")

# include_directories(
# 	${CMAKE_CURRENT_SOURCE_DIR}/include
# 	${OpenCV_INCLUDE_DIRS}
# )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_VERBOSE_MAKEFILE ON)

add_compile_options(
	-Wall -Wextra -msse3
	$<$<CONFIG:RELEASE>:-Ofast>
	$<$<CONFIG:DEBUG>:-O0>
	$<$<CONFIG:DEBUG>:--ggdb3>
)

add_compile_definitions(
	$<$<CONFIG:RELEASE>:NDEBUG>
)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(rtsp_ai_app ${SOURCES})
target_link_libraries(rtsp_ai_app PRIVATE ${OpenCV_LIBS})
target_link_libraries(rtsp_ai_app PRIVATE Threads:Threads)
