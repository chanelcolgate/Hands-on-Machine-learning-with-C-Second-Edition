cmake_minimum_required(VERSION 3.20)
project(rtsp_ai_framework LANGUAGES CXX)

# Find required packages
find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)


# Include directories
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${OpenCV_INCLUDE_DIRS}
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

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

# file(GLOB_RECURSE SOURCES "src/*.cpp")
# 
# add_executable(rtsp_ai_app ${SOURCES})

# Source files
set(CORE_SOURCES src/main.cpp)

# Create the main executable
add_executable(rtsp_ai_app ${CORE_SOURCES})

# Link libraries
target_link_libraries(rtsp_ai_app PRIVATE ${OpenCV_LIBS} Threads::Threads)

# Optional: Add compiler warnings
if(MSVC)
	target_compile_options(rtsp_ai_app PRIVATE /W4)
else()
	target_compile_options(rtsp_ai_app PRIVATE -Wall -Wextra -pedantic)
endif()

# Optional: Enable address sanitizer for development
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_options(rtsp_ai_app PRIVATE -fsanitize=address -g)
	target_link_options(rtsp_ai_app PRIVATE -fsanitize=address)
endif()

# ============ UNIT TESTS ============

# Google Test (if available)
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# Để tránh gtest thay đổi runtime flags của compiler
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# Tắt tính năng build test nội bộ của chính Google Test để biên dịch nhanh hơn
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)
# 
# # Tạo target test của bạn
# add_executable(unit_tests test/main_test.cpp)
# target_link_libraries(unit_tests PRIVATE gtest_main)

# FIX LỖI CYGWIN: Bỏ __STRICT_ANSI__ và định nghĩa _GNU_SOURCE để bật POSIX mkstemp()
if(CYGWIN OR MINGW)
  target_compile_definitions(gtest PRIVATE _GNU_SOURCE _DEFAULT_SOURCE)
  target_compile_definitions(gtest_main PRIVATE _GNU_SOURCE _DEFAULT_SOURCE)
  target_compile_options(gtest PRIVATE -U__STRICT_ANSI__)
  target_compile_options(gtest_main PRIVATE -U__STRICT_ANSI__)
endif()

add_executable(test_safe_queue tests/test_safe_queue.cpp)
add_executable(test_frame_data tests/test_frame_data.cpp)
add_executable(test_preprocessor tests/test_preprocessor.cpp)

if(CYGWIN OR MINGW)
	target_compile_options(test_safe_queue PRIVATE -fext-numeric-literals)
	target_compile_options(test_frame_data PRIVATE -fext-numeric-literals)
	target_compile_options(test_preprocessor PRIVATE -fext-numeric-literals)
endif()

target_link_libraries(test_safe_queue
	PRIVATE
		gtest_main
		Threads::Threads
		${OpenCV_LIBS}
)
target_link_libraries(test_frame_data
	PRIVATE
		gtest_main
		Threads::Threads
		${OpenCV_LIBS}
)
target_link_libraries(test_preprocessor
	PRIVATE
		gtest_main
		Threads::Threads
		${OpenCV_LIBS}
)


# Enable testing
enable_testing()
add_test(NAME TestSafeQueue COMMAND test_safe_queue)
add_test(NAME TestFrameData COMMAND test_frame_data)
add_test(NAME TestPreprocessor COMMAND test_preprocessor)


# Print build information
message(STATUS "RTSP AI Framework Build Configuration:")
message(STATUS "  OpenCV version: ${OpenCV_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER}")
