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

# Check if Holoscan SDK is available
if ! pkg-config --exists holoscan; then
    echo -e "${RED}Error: Holoscan SDK not found${NC}"
    echo "Please install Holoscan SDK and ensure it's in your PKG_CONFIG_PATH"
    exit 1
fi

# Check for required dependencies
echo -e "${YELLOW}Checking dependencies...${NC}"

# Check for OpenCV
if ! pkg-config --exists opencv4; then
    echo -e "${RED}Error: OpenCV not found${NC}"
    echo "Please install OpenCV development packages"
    exit 1
fi

# Check for yaml-cpp
if ! pkg-config --exists yaml-cpp; then
    echo -e "${RED}Error: yaml-cpp not found${NC}"
    echo "Please install yaml-cpp development packages"
    exit 1
fi

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