#!/bin/bash

# Build script for Urology Inference Holoscan C++
# Usage: ./scripts/build.sh [clean|debug|release]

set -e

# Default build type
BUILD_TYPE="Release"
CLEAN_BUILD=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        clean)
            CLEAN_BUILD=true
            shift
            ;;
        debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        release)
            BUILD_TYPE="Release"
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [clean|debug|release]"
            echo "  clean   - Clean build directory before building"
            echo "  debug   - Build in debug mode"
            echo "  release - Build in release mode (default)"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}Urology Inference Holoscan C++ Build Script${NC}"
echo "=========================================="

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

echo "Project directory: $PROJECT_DIR"
echo "Build directory: $BUILD_DIR"
echo "Build type: $BUILD_TYPE"

# Check if we're running in a container
if [ -f /.dockerenv ]; then
    echo -e "${GREEN}Running in Docker container${NC}"
else
    echo -e "${YELLOW}Not running in Docker container, attempting to run in Holoscan container...${NC}"
    # Try to run the build script inside the Holoscan container
    docker run --rm -it \
        -v "$(pwd):/workspace" \
        -w /workspace \
        --runtime=nvidia \
        --ipc=host \
        --cap-add=CAP_SYS_PTRACE \
        --ulimit memlock=-1 \
        --ulimit stack=67108864 \
        nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu \
        /bin/bash -c "chmod +x /workspace/scripts/build.sh && /workspace/scripts/build.sh"
    exit $?
fi

# Check if Holoscan SDK is available
echo -e "${YELLOW}Checking Holoscan SDK...${NC}"
if [ ! -d "/opt/nvidia/holoscan" ]; then
    echo -e "${RED}Error: Holoscan SDK not found at /opt/nvidia/holoscan${NC}"
    echo "Please ensure you are running in the Holoscan container"
    exit 1
fi
echo -e "${GREEN}✓ Holoscan SDK found at /opt/nvidia/holoscan${NC}"

# Check for required dependencies
echo -e "${YELLOW}Checking dependencies...${NC}"

# In Holoscan container, dependencies should already be installed
echo -e "${GREEN}✓ Dependencies should be available in Holoscan container${NC}"

# Check for video encoder GXF extensions
echo -e "${YELLOW}Checking GXF video encoder extensions...${NC}"
HOLOSCAN_LIBS_DIR="/opt/nvidia/holoscan/lib/"
GXF_EXTENSIONS=("libgxf_videoencoder.so" "libgxf_videoencoderio.so" "libgxf_videodecoder.so" "libgxf_videodecoderio.so")

missing_extensions=()
for ext in "${GXF_EXTENSIONS[@]}"; do
    if [[ -f "$HOLOSCAN_LIBS_DIR$ext" ]]; then
        echo -e "${GREEN}✓ Found $ext${NC}"
    else
        echo -e "${YELLOW}⚠ Missing $ext${NC}"
        missing_extensions+=("$ext")
    fi
done

if [[ ${#missing_extensions[@]} -gt 0 ]]; then
    echo -e "${YELLOW}Warning: Some GXF video encoder extensions are missing.${NC}"
    echo "Video recording functionality may not work properly."
    echo "To install missing extensions, run:"
    echo "  $SCRIPT_DIR/install_video_encoder_deps.sh"
    echo ""
fi

echo -e "${GREEN}Dependencies check completed${NC}"

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Set CMAKE_PREFIX_PATH for Holoscan
export CMAKE_PREFIX_PATH="/opt/nvidia/holoscan${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"

# Configure with CMake
echo -e "${YELLOW}Configuring build...${NC}"
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      "$PROJECT_DIR"

# Build
echo -e "${YELLOW}Building...${NC}"
CPU_COUNT=$(nproc)
make -j"$CPU_COUNT"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build completed successfully!${NC}"
    echo ""
    echo "Executables:"
    find "$BUILD_DIR" -name "urology_inference_holoscan_cpp*" -executable -type f
    echo ""
    echo "To run the application:"
    echo "  cd $BUILD_DIR"
    echo "  ./urology_inference_holoscan_cpp --help"
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi 