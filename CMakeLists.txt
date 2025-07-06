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

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(yaml-cpp QUIET)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${OpenCV_INCLUDE_DIRS})

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

# Main executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Minimal test executable
add_executable(minimal_urology_app src/simple_minimal_app.cpp)

# Hello world test
add_executable(hello_world src/hello_world.cpp)

# Simple inference test (no HolovizOp)
add_executable(simple_inference_test src/simple_inference_test.cpp)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    holoscan::core
    holoscan::ops::video_stream_replayer
    holoscan::ops::format_converter
    holoscan::ops::inference
    holoscan::ops::holoviz
    holoscan::ops::gxf_codelet
    ${OpenCV_LIBS}
    Threads::Threads
    CUDA::cudart
    CUDA::cuda_driver
)

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
    ${OpenCV_LIBS}
)

# Add yaml-cpp if found
if(yaml-cpp_FOUND)
    target_link_libraries(${PROJECT_NAME} yaml-cpp)
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

# Set properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(minimal_urology_app PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Print configuration info
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
if(yaml-cpp_FOUND)
    message(STATUS "yaml-cpp found")
else()
    message(STATUS "yaml-cpp NOT found")
endif() 