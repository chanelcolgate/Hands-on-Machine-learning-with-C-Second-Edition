cmake_minimum_required(VERSION 3.20)

set(PROJECT_NAME rtsp_ai_app)
project(${PROJECT_NAME} LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(Boost_NO_SYSTEM_PATHS ON)

# Find PkgConfig & Threads
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# 1. FIX LỖI OPENCV TRÊN CYGWIN: Dùng PkgConfig thay vì find_package(OpenCV)
# Tránh lỗi đường dẫn "//include" trong OpenCVConfig.cmake của Cygwin
pkg_check_modules(OPENCV REQUIRED IMPORTED_TARGET opencv)

# 2. Tìm các module FFmpeg
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libswscale
    libavutil
)

add_library(Eigen INTERFACE)
target_include_directories(Eigen INTERFACE
    $<BUILD_INTERFACE:/home/MIND_SUPPORT/Documents/libs/eigen>
    $<INSTALL_INTERFACE:include>
)

add_library(Boost_lib INTERFACE)
target_include_directories(Boost_lib INTERFACE
    $<BUILD_INTERFACE:/home/MIND_SUPPORT/Documents/libs/boost>
    $<INSTALL_INTERFACE:include>
)

# Include directories toàn cục
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_compile_options(
    -Wall -Wextra -msse3
    $<$<CONFIG:RELEASE>:-Ofast>
    $<$<CONFIG:DEBUG>:-O0>
    $<$<CONFIG:DEBUG>:--ggdb3>
)

add_compile_definitions(
    $<$<CONFIG:RELEASE>:NDEBUG>
)

# Source files
set(CORE_SOURCES src/main.cpp)

# Create the main executable
add_executable(${PROJECT_NAME} ${CORE_SOURCES})

# Link OpenCV + FFmpeg + Threads cho app chính
target_link_libraries(${PROJECT_NAME}
    PRIVATE 
        PkgConfig::OPENCV
        PkgConfig::FFMPEG
        Threads::Threads
        Eigen
)

# Compiler options cho app chính
if(MSVC)
    target_compile_options(rtsp_ai_app PRIVATE /W4)
else()
    target_compile_options(rtsp_ai_app PRIVATE -Wall -Wextra -pedantic)
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Enable address sanitizer cho Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(rtsp_ai_app PRIVATE -fsanitize=address -g)
    target_link_options(rtsp_ai_app PRIVATE -fsanitize=address)
endif()

# ============ UNIT TESTS ============

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# Cấu hình Google Test
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# FIX LỖI CYGWIN: Bỏ __STRICT_ANSI__ và định nghĩa _GNU_SOURCE cho gtest
if(CYGWIN OR MINGW)
  target_compile_definitions(gtest PRIVATE _GNU_SOURCE _DEFAULT_SOURCE)
  target_compile_definitions(gtest_main PRIVATE _GNU_SOURCE _DEFAULT_SOURCE)
  target_compile_options(gtest PRIVATE -U__STRICT_ANSI__)
  target_compile_options(gtest_main PRIVATE -U__STRICT_ANSI__)
endif()

add_executable(test_safe_queue tests/test_safe_queue.cpp)
add_executable(test_frame_data tests/test_frame_data.cpp)
add_executable(test_preprocessor tests/test_preprocessor.cpp)
add_executable(test_ffmpeg_rtsp_reader tests/test_ffmpeg_rtsp_reader.cpp)
add_executable(test_display_monitor tests/test_display_monitor.cpp)
add_executable(test_function_sigmoid tests/test_function_sigmoid.cpp)
add_executable(test_find_middle tests/test_find_middle.cpp)
add_executable(test_glorot tests/test_glorot.cpp)

if(CYGWIN OR MINGW)
    target_compile_options(test_safe_queue PRIVATE -fext-numeric-literals)
    target_compile_options(test_frame_data PRIVATE -fext-numeric-literals)
    target_compile_options(test_preprocessor PRIVATE -fext-numeric-literals)
    target_compile_options(test_ffmpeg_rtsp_reader PRIVATE -fext-numeric-literals)
    target_compile_options(test_display_monitor PRIVATE -fext-numeric-literals)
    target_compile_options(test_function_sigmoid PRIVATE -fext-numeric-literals)
    target_compile_options(test_find_middle PRIVATE -fext-numeric-literals)
    target_compile_options(test_glorot PRIVATE -fext-numeric-literals)
endif()

target_link_libraries(test_safe_queue
    PRIVATE
        gtest_main
        Threads::Threads
        PkgConfig::OPENCV
)
target_link_libraries(test_frame_data
    PRIVATE
        gtest_main
        Threads::Threads
        PkgConfig::OPENCV
)
target_link_libraries(test_preprocessor
    PRIVATE
        gtest_main
        Threads::Threads
        PkgConfig::OPENCV
)
target_link_libraries(test_ffmpeg_rtsp_reader
    PRIVATE
        gtest_main
        PkgConfig::FFMPEG
        PkgConfig::OPENCV
        Threads::Threads
)
target_link_libraries(test_display_monitor
    PRIVATE
        gtest_main
        PkgConfig::FFMPEG
        PkgConfig::OPENCV
        Threads::Threads
)
target_link_libraries(test_function_sigmoid
    PRIVATE
        gtest_main
        PkgConfig::FFMPEG
        PkgConfig::OPENCV
        Threads::Threads
        Eigen)

target_link_libraries(test_find_middle
    PRIVATE
        gtest_main
        PkgConfig::FFMPEG
        PkgConfig::OPENCV
        Threads::Threads
        Eigen)

target_link_libraries(test_glorot
    PRIVATE
        gtest_main
        PkgConfig::FFMPEG
        PkgConfig::OPENCV
        Threads::Threads
        Eigen
        Boost_lib)

# Enable testing
enable_testing()
add_test(NAME TestSafeQueue COMMAND test_safe_queue)
add_test(NAME TestFrameData COMMAND test_frame_data)
add_test(NAME TestPreprocessor COMMAND test_preprocessor)
add_test(NAME TestFFmpegRtspReader COMMAND test_ffmpeg_rtsp_reader)
add_test(NAME TestDisplayMonitor COMMAND test_display_monitor)
add_test(NAME TestFunctionSigmoid COMMAND test_function_sigmoid)
add_test(NAME TestFindMiddle COMMAND test_find_middle)
add_test(NAME TestGlorot COMMAND test_glorot)

# Print build information
message(STATUS "RTSP AI Framework Build Configuration:")
message(STATUS "  OpenCV Target: PkgConfig::OPENCV")
message(STATUS "  FFmpeg Target: PkgConfig::FFMPEG")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
