Để chia thư mục test cho đỡ nhầm lẫn, bạn cần tạo thư mục con trong `tests/` và sửa `CMakeLists.txt`. Dưới đây là cách tổ chức:Perfect! Bạn có thể chia thư mục test thành các subfolder để organize tốt hơn. Dưới đây là cách sửa `CMakeLists.txt`:

## 📁 Cấu trúc thư mục test đề xuất

```
tests/
├── unit/                    ← Unit tests
│   ├── test_safe_queue.cpp
│   ├── test_frame_data.cpp
│   ├── test_preprocessor.cpp
│   └── test_convolution.cpp
├── integration/             ← Integration tests
│   ├── test_ffmpeg_rtsp_reader.cpp
│   └── test_display_monitor.cpp
└── math/                    ← Math function tests
    ├── test_function_sigmoid.cpp
    ├── test_find_middle.cpp
    ├── test_glorot.cpp
    └── test_convolution.cpp
```

## ✅ Sửa CMakeLists.txt

```cmake
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

# ========== UNIT TESTS ==========
add_executable(test_safe_queue tests/unit/test_safe_queue.cpp)
add_executable(test_frame_data tests/unit/test_frame_data.cpp)
add_executable(test_preprocessor tests/unit/test_preprocessor.cpp)
add_executable(test_convolution tests/unit/test_convolution.cpp)

# ========== INTEGRATION TESTS ==========
add_executable(test_ffmpeg_rtsp_reader tests/integration/test_ffmpeg_rtsp_reader.cpp)
add_executable(test_display_monitor tests/integration/test_display_monitor.cpp)

# ========== MATH TESTS ==========
add_executable(test_function_sigmoid tests/math/test_function_sigmoid.cpp)
add_executable(test_find_middle tests/math/test_find_middle.cpp)
add_executable(test_glorot tests/math/test_glorot.cpp)

# ========== APPLY CYGWIN/MINGW FIX ==========
set(ALL_TESTS 
    test_safe_queue
    test_frame_data
    test_preprocessor
    test_convolution
    test_ffmpeg_rtsp_reader
    test_display_monitor
    test_function_sigmoid
    test_find_middle
    test_glorot
)

if(CYGWIN OR MINGW)
    foreach(test ${ALL_TESTS})
        target_compile_options(${test} PRIVATE -fext-numeric-literals)
    endforeach()
endif()

# ========== LINK LIBRARIES ==========

# Unit Tests
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

target_link_libraries(test_convolution
    PRIVATE
        gtest_main
        Threads::Threads
        PkgConfig::OPENCV
)

# Integration Tests
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

# Math Tests
target_link_libraries(test_function_sigmoid
    PRIVATE
        gtest_main
        PkgConfig::FFMPEG
        PkgConfig::OPENCV
        Threads::Threads
        Eigen
)

target_link_libraries(test_find_middle
    PRIVATE
        gtest_main
        PkgConfig::FFMPEG
        PkgConfig::OPENCV
        Threads::Threads
        Eigen
)

target_link_libraries(test_glorot
    PRIVATE
        gtest_main
        PkgConfig::FFMPEG
        PkgConfig::OPENCV
        Threads::Threads
        Eigen
        Boost_lib
)

# ========== ENABLE TESTING ==========
enable_testing()

# Unit Tests
add_test(NAME TestSafeQueue COMMAND test_safe_queue)
add_test(NAME TestFrameData COMMAND test_frame_data)
add_test(NAME TestPreprocessor COMMAND test_preprocessor)
add_test(NAME TestConvolution COMMAND test_convolution)

# Integration Tests
add_test(NAME TestFFmpegRtspReader COMMAND test_ffmpeg_rtsp_reader)
add_test(NAME TestDisplayMonitor COMMAND test_display_monitor)

# Math Tests
add_test(NAME TestFunctionSigmoid COMMAND test_function_sigmoid)
add_test(NAME TestFindMiddle COMMAND test_find_middle)
add_test(NAME TestGlorot COMMAND test_glorot)

# Print build information
message(STATUS "RTSP AI Framework Build Configuration:")
message(STATUS "  OpenCV Target: PkgConfig::OPENCV")
message(STATUS "  FFmpeg Target: PkgConfig::FFMPEG")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
```

## 🎯 Cách sử dụng

**Chạy tất cả tests:**
```bash
cd rtsp_ai_framework && mkdir build && cd build
cmake ..
make
ctest
```

**Chạy riêng từng test:**
```bash
# Unit tests
./test_convolution

# Math tests
./test_glorot
./test_function_sigmoid

# Integration tests
./test_ffmpeg_rtsp_reader
```

**Chạy tests theo category:**
```bash
# Label testing (yêu cầu CMake 3.21+)
ctest -L "Unit"
ctest -L "Math"
ctest -L "Integration"
```

## 📝 Thêm labels vào tests (nâng cao)

Để organize tests theo category:

```cmake
# Unit Tests
add_test(NAME TestSafeQueue COMMAND test_safe_queue)
set_tests_properties(TestSafeQueue PROPERTIES LABELS "Unit")

add_test(NAME TestConvolution COMMAND test_convolution)
set_tests_properties(TestConvolution PROPERTIES LABELS "Unit")

# Math Tests
add_test(NAME TestGlorot COMMAND test_glorot)
set_tests_properties(TestGlorot PROPERTIES LABELS "Math")

# Integration Tests
add_test(NAME TestFFmpegRtspReader COMMAND test_ffmpeg_rtsp_reader)
set_tests_properties(TestFFmpegRtspReader PROPERTIES LABELS "Integration")
```

Rồi chạy theo label:
```bash
ctest -L Unit      # Chỉ chạy unit tests
ctest -L Math      # Chỉ chạy math tests
```
