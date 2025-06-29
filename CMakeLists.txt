cmake_minimum_required(VERSION 3.20)
project(urology_inference_holoscan_cpp LANGUAGES C CXX)

# C++ Standard and Compiler Options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type specific flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Common flags for GCC and Clang
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
    
    # Additional performance flags
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto -ffast-math")
    endif()
endif()

# CUDA specific optimizations
if(CUDA_FOUND OR CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode arch=compute_70,code=sm_70")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode arch=compute_75,code=sm_75")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode arch=compute_80,code=sm_80")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode arch=compute_86,code=sm_86")
endif()

# Find required packages
find_package(holoscan 2.1 REQUIRED CONFIG
             PATHS "/opt/nvidia/holoscan" "/workspace/holoscan-sdk/install")

# Create version header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/version.hpp.in 
               ${CMAKE_CURRENT_BINARY_DIR}/include/version.hpp @ONLY)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

# Find Qt6 (optional for GUI)
find_package(Qt6 COMPONENTS Core Widgets QUIET)

# OpenCV for image processing
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)

# Sources - Core Application
set(CORE_SOURCE_FILES
    src/main.cpp
    src/urology_app.cpp
)

# Sources - Operators
set(OPERATOR_SOURCE_FILES
    src/operators/yolo_seg_postprocessor.cpp
    src/operators/dummy_receiver.cpp
    src/operators/passthrough.cpp
    src/operators/video_encoder_ops.cpp
)

# Sources - Utilities
set(UTILITY_SOURCE_FILES
    src/utils/yolo_utils.cpp
    src/utils/cv_utils.cpp
    src/utils/logger.cpp
    src/utils/performance_monitor.cpp
    src/utils/error_handler.cpp
)

# Sources - Configuration
set(CONFIG_SOURCE_FILES
    src/config/app_config.cpp
    src/config/app_config_new.cpp
)

# Combine all source files
set(SOURCE_FILES
    ${CORE_SOURCE_FILES}
    ${OPERATOR_SOURCE_FILES}
    ${UTILITY_SOURCE_FILES}
    ${CONFIG_SOURCE_FILES}
)

# Qt GUI sources (optional)
if(Qt6_FOUND)
    set(QT_SOURCE_FILES
        src/widgets/main_window.cpp
        src/widgets/rounded_window.cpp
        src/qt_main.cpp
    )
    qt6_standard_project_setup()
endif()

# Main executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    holoscan::core
    holoscan::ops::inference
    holoscan::ops::holoviz  
    holoscan::ops::video_stream_replayer
    holoscan::ops::format_converter
    holoscan::ops::segmentation_postprocessor
    ${OpenCV_LIBS}
    yaml-cpp
)

# Qt GUI executable (optional)
if(Qt6_FOUND)
    add_executable(${PROJECT_NAME}_qt ${SOURCE_FILES} ${QT_SOURCE_FILES})
    target_link_libraries(${PROJECT_NAME}_qt 
        holoscan::core
        holoscan::ops::inference
        holoscan::ops::holoviz
        holoscan::ops::video_stream_replayer
        holoscan::ops::format_converter
        holoscan::ops::segmentation_postprocessor
        ${OpenCV_LIBS}
        yaml-cpp
        Qt6::Core
        Qt6::Widgets
    )
    target_compile_definitions(${PROJECT_NAME}_qt PRIVATE ENABLE_QT_GUI)
endif()

# Install targets
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
if(Qt6_FOUND)
    install(TARGETS ${PROJECT_NAME}_qt DESTINATION bin)
endif()

# Install configuration files
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/config/ 
        DESTINATION share/${PROJECT_NAME}/config
        FILES_MATCHING PATTERN "*.yaml")

# Testing support
option(ENABLE_TESTING "Enable unit testing" OFF)
option(ENABLE_BENCHMARKS "Enable performance benchmarks" OFF)

if(ENABLE_TESTING)
    enable_testing()
    find_package(GTest REQUIRED)
    find_package(GMock REQUIRED)
    
    # Unit tests
    add_executable(unit_tests
        tests/unit/test_config.cpp
        ${CONFIG_SOURCE_FILES}
        ${UTILITY_SOURCE_FILES}
    )
    
    target_link_libraries(unit_tests
        ${OpenCV_LIBS}
        yaml-cpp
        GTest::gtest
        GTest::gtest_main
        GMock::gmock
        GMock::gmock_main
    )
    
    target_include_directories(unit_tests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
    )
    
    add_test(NAME UnitTests COMMAND unit_tests)
endif()

if(ENABLE_BENCHMARKS)
    find_package(benchmark REQUIRED)
    
    # Performance benchmarks
    add_executable(performance_benchmarks
        tests/performance/benchmark_yolo.cpp
        ${OPERATOR_SOURCE_FILES}
        ${UTILITY_SOURCE_FILES}
        ${CONFIG_SOURCE_FILES}
    )
    
    target_link_libraries(performance_benchmarks
        holoscan::core
        holoscan::ops::inference
        ${OpenCV_LIBS}
        yaml-cpp
        benchmark::benchmark
        benchmark::benchmark_main
    )
    
    target_include_directories(performance_benchmarks PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
    )
endif()

# Static analysis
option(ENABLE_STATIC_ANALYSIS "Enable static code analysis" OFF)
if(ENABLE_STATIC_ANALYSIS)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    endif()
endif()

# Code coverage
option(ENABLE_COVERAGE "Enable code coverage analysis" OFF)
if(ENABLE_COVERAGE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${PROJECT_NAME} PRIVATE --coverage)
        target_link_libraries(${PROJECT_NAME} --coverage)
    endif()
endif() 