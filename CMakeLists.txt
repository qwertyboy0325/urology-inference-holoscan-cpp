cmake_minimum_required(VERSION 3.20)
project(urology_inference_holoscan_cpp VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable CUDA support
enable_language(CUDA)
find_package(CUDAToolkit REQUIRED)

# Set CUDA properties
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)

# Find required packages
find_package(holoscan REQUIRED CONFIG
    PATHS "/opt/nvidia/holoscan"
)

# Make OpenCV optional since it's not available in Holoscan container
find_package(OpenCV QUIET)
find_package(PkgConfig QUIET)
find_package(Threads REQUIRED)
find_package(yaml-cpp QUIET)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
if(OpenCV_FOUND)
include_directories(${OpenCV_INCLUDE_DIRS})
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
else()
    message(STATUS "OpenCV not found - cv_utils will be disabled")
    add_definitions(-DDISABLE_OPENCV)
endif()

# Collect source files
file(GLOB_RECURSE SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cu"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cu"
)

# Remove test files and specific executables from main build
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*hello_world\\.cpp$")
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*simple_minimal_app\\.cpp$")
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*simple_test\\.cpp$")
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*simple_inference_test\\.cpp$")
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*test_without_postprocessor\\.cpp$")
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*test_postprocessor_only\\.cpp$")
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*plug_and_play_yolo_postprocessor\\.cpp$")

# Main executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Minimal test executable
add_executable(minimal_urology_app src/simple_minimal_app.cpp)

# Hello world test
add_executable(hello_world src/hello_world.cpp)

# Simple inference test (no HolovizOp)
add_executable(simple_inference_test 
    src/simple_inference_test.cpp
    src/operators/yolo_seg_postprocessor.cpp
    src/operators/yolo_seg_postprocessor.cu
    src/utils/yolo_utils.cpp
    src/utils/logger.cpp
    src/utils/error_handler.cpp
    src/utils/performance_monitor.cpp
    src/utils/gxf_state_monitor.cpp
    src/utils/cv_utils.cpp
)

# Test without postprocessor
add_executable(test_without_postprocessor 
    src/test_without_postprocessor.cpp
)

# Test postprocessor only (no HolovizOp)
add_executable(test_postprocessor_only 
    src/test_postprocessor_only.cpp
    src/operators/yolo_seg_postprocessor.cpp
    src/operators/yolo_seg_postprocessor.cu
    src/utils/yolo_utils.cpp
    src/utils/logger.cpp
    src/utils/error_handler.cpp
    src/utils/performance_monitor.cpp
    src/utils/gxf_state_monitor.cpp
    src/utils/cv_utils.cpp
)

# NEW: HolovizOp Integration Test
add_executable(holoviz_integration_test 
    tests/integration/test_holoviz_integration.cpp
    src/operators/holoviz_native_yolo_postprocessor.cpp
    src/utils/holoviz_compatible_tensors.cpp
    src/utils/holoviz_static_config.cpp
    src/utils/yolo_utils.cpp
    src/utils/logger.cpp
    src/utils/error_handler.cpp
    src/utils/performance_monitor.cpp
)

# NEW: Unit Tests
enable_testing()

# Unit test for HolovizOp-native postprocessor
add_executable(test_holoviz_native_postprocessor
    tests/unit/test_holoviz_native_postprocessor.cpp
    src/operators/holoviz_native_yolo_postprocessor.cpp
    src/utils/holoviz_compatible_tensors.cpp
    src/utils/holoviz_static_config.cpp
    src/utils/yolo_utils.cpp
    src/utils/logger.cpp
    src/utils/error_handler.cpp
    src/utils/performance_monitor.cpp
)

# Unit test for HolovizOp foundation components
add_executable(test_holoviz_foundation
    tests/unit/test_holoviz_foundation.cpp
    src/utils/holoviz_compatible_tensors.cpp
    src/utils/holoviz_static_config.cpp
    src/utils/yolo_utils.cpp
    src/utils/logger.cpp
    src/utils/error_handler.cpp
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    holoscan::core
    holoscan::ops::video_stream_replayer
    holoscan::ops::format_converter
    holoscan::ops::inference
    holoscan::ops::holoviz
    holoscan::ops::gxf_codelet
    Threads::Threads
    CUDA::cudart
    CUDA::cuda_driver
)

# Link OpenCV if found
if(OpenCV_FOUND)
    target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
endif()

# Link libraries for minimal app
target_link_libraries(minimal_urology_app
    holoscan::core
    holoscan::ops::video_stream_replayer
    holoscan::ops::format_converter
)

# Link libraries for simple inference test
target_link_libraries(simple_inference_test
    holoscan::core
    holoscan::ops::video_stream_replayer
    holoscan::ops::format_converter
    holoscan::ops::inference
    holoscan::ops::gxf_codelet
    holoscan::ops::holoviz
    ${OpenCV_LIBS}
    CUDA::cudart
    CUDA::cuda_driver
)

# Link libraries for test without postprocessor
target_link_libraries(test_without_postprocessor
    holoscan::core
    holoscan::ops::video_stream_replayer
    holoscan::ops::format_converter
    holoscan::ops::inference
    holoscan::ops::holoviz
    CUDA::cudart
    CUDA::cuda_driver
)

# Link libraries for test postprocessor only
target_link_libraries(test_postprocessor_only
    holoscan::core
    holoscan::ops::video_stream_replayer
    holoscan::ops::format_converter
    holoscan::ops::inference
    holoscan::ops::holoviz
    holoscan::ops::gxf_codelet
    ${OpenCV_LIBS}
    CUDA::cudart
    CUDA::cuda_driver
)

# NEW: Link libraries for HolovizOp integration test
target_link_libraries(holoviz_integration_test
    holoscan::core
    holoscan::ops::video_stream_replayer
    holoscan::ops::format_converter
    holoscan::ops::inference
    holoscan::ops::holoviz
    holoscan::ops::gxf_codelet
    Threads::Threads
    CUDA::cudart
    CUDA::cuda_driver
)

# NEW: Link libraries for unit tests
target_link_libraries(test_holoviz_native_postprocessor
    holoscan::core
    holoscan::ops::holoviz
    holoscan::ops::gxf_codelet
    Threads::Threads
    CUDA::cudart
    CUDA::cuda_driver
)

target_link_libraries(test_holoviz_foundation
    holoscan::core
    Threads::Threads
)

# Add yaml-cpp if found
if(yaml-cpp_FOUND)
    target_link_libraries(${PROJECT_NAME} yaml-cpp)
    target_link_libraries(holoviz_integration_test yaml-cpp)
endif()

# Set compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -O3
    -DWITH_CUDA
)

target_compile_options(minimal_urology_app PRIVATE
    -Wall
    -Wextra
    -O3
)

# NEW: Set compiler flags for tests
target_compile_options(holoviz_integration_test PRIVATE
    -Wall
    -Wextra
    -O3
    -DWITH_CUDA
)

target_compile_options(test_holoviz_native_postprocessor PRIVATE
    -Wall
    -Wextra
    -O3
    -DWITH_CUDA
)

target_compile_options(test_holoviz_foundation PRIVATE
    -Wall
    -Wextra
    -O3
)

# Set properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(minimal_urology_app PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# NEW: Set properties for tests
set_target_properties(holoviz_integration_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(test_holoviz_native_postprocessor PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(test_holoviz_foundation PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# NEW: Add tests to CTest
add_test(NAME test_holoviz_native_postprocessor COMMAND test_holoviz_native_postprocessor)
add_test(NAME test_holoviz_foundation COMMAND test_holoviz_foundation)
add_test(NAME holoviz_integration_test COMMAND holoviz_integration_test)

# Print configuration info
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
if(yaml-cpp_FOUND)
    message(STATUS "yaml-cpp found")
else()
    message(STATUS "yaml-cpp NOT found")
endif()

# NEW: Print HolovizOp-native components info
message(STATUS "HolovizOp-native components enabled:")
message(STATUS "  - HolovizNativeYoloPostprocessor")
message(STATUS "  - HolovizCompatibleTensors")
message(STATUS "  - HolovizTensorFactory")
message(STATUS "  - HolovizStaticConfig")
message(STATUS "  - Integration tests")
message(STATUS "  - Unit tests") 