#!/bin/bash

# Optimized build script for urology inference holoscan cpp
# Features: Multi-threaded builds, dependency checking, performance optimizations,
#          comprehensive logging, build caching, and more

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Release"
CLEAN_BUILD=false
INSTALL_DEPS=false
BUILD_DIR="build"
PARALLEL_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
ENABLE_TESTING=false
ENABLE_BENCHMARKS=false
VERBOSE=false
INSTALL_TARGET=false
ENABLE_LTO=true
ENABLE_STATIC_ANALYSIS=false
CREATE_PACKAGE=false
USE_CCACHE=true
PROFILE_BUILD=false

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Logging functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_debug() { [[ "$VERBOSE" == true ]] && echo -e "${CYAN}[DEBUG]${NC} $1"; }

# Performance profiling
start_timer() {
    TIMER_START=$(date +%s)
}

end_timer() {
    TIMER_END=$(date +%s)
    ELAPSED=$((TIMER_END - TIMER_START))
    log_info "Operation completed in ${ELAPSED}s"
}

# Dependency checking
check_dependencies() {
    log_info "Checking build dependencies..."
    start_timer
    
    local missing_deps=()
    
    # Essential tools
    for tool in cmake make git; do
        if ! command -v "$tool" &> /dev/null; then
            missing_deps+=("$tool")
        fi
    done
    
    # Compilers
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        missing_deps+=("g++ or clang++")
    fi
    
    # Optional tools
    if [[ "$USE_CCACHE" == true ]] && ! command -v ccache &> /dev/null; then
        log_warning "ccache not found - builds will be slower"
        USE_CCACHE=false
    fi
    
    if [[ "$ENABLE_STATIC_ANALYSIS" == true ]] && ! command -v clang-tidy &> /dev/null; then
        log_warning "clang-tidy not found - static analysis disabled"
        ENABLE_STATIC_ANALYSIS=false
    fi
    
    # CUDA (optional)
    if command -v nvcc &> /dev/null; then
        local cuda_version=$(nvcc --version | grep "release" | sed 's/.*release \([0-9]\+\.[0-9]\+\).*/\1/')
        log_info "CUDA compiler found: version $cuda_version"
    else
        log_warning "CUDA compiler not found - GPU acceleration may be limited"
    fi
    
    # Report missing dependencies
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        log_error "Missing required dependencies: ${missing_deps[*]}"
        exit 1
    fi
    
    end_timer
    log_success "All required dependencies found"
}

# Package-specific dependency checking
check_holoscan_dependencies() {
    log_info "Checking Holoscan SDK dependencies..."
    
    # Check for Holoscan SDK
    if ! pkg-config --exists holoscan 2>/dev/null; then
        log_error "Holoscan SDK not found"
        log_info "Please install Holoscan SDK and ensure it's in your PKG_CONFIG_PATH"
        exit 1
    fi
    
    local holoscan_version=$(pkg-config --modversion holoscan)
    log_info "Holoscan SDK found: version $holoscan_version"
    
    # Check for OpenCV
    if ! pkg-config --exists opencv4 2>/dev/null; then
        log_error "OpenCV not found"
        exit 1
    fi
    
    local opencv_version=$(pkg-config --modversion opencv4)
    log_info "OpenCV found: version $opencv_version"
    
    # Check for yaml-cpp
    if ! pkg-config --exists yaml-cpp 2>/dev/null; then
        log_error "yaml-cpp not found"
        exit 1
    fi
    
    # Check GXF extensions
    local holoscan_libs_dir="/opt/nvidia/holoscan/lib/"
    local gxf_extensions=("libgxf_videoencoder.so" "libgxf_videoencoderio.so")
    
    for ext in "${gxf_extensions[@]}"; do
        if [[ -f "$holoscan_libs_dir$ext" ]]; then
            log_debug "Found GXF extension: $ext"
        else
            log_warning "Missing GXF extension: $ext"
        fi
    done
    
    log_success "Holoscan dependencies verified"
}

# System optimization
optimize_build_environment() {
    log_info "Optimizing build environment..."
    
    # Set optimal parallel jobs
    local mem_gb=$(($(free -m | grep '^Mem:' | awk '{print $2}') / 1024))
    local optimal_jobs=$((mem_gb / 2))  # ~2GB per job for C++
    
    if [[ $optimal_jobs -lt $PARALLEL_JOBS ]]; then
        log_warning "Reducing parallel jobs from $PARALLEL_JOBS to $optimal_jobs due to memory constraints"
        PARALLEL_JOBS=$optimal_jobs
    fi
    
    # Set compiler cache
    if [[ "$USE_CCACHE" == true ]]; then
        export CC="ccache gcc"
        export CXX="ccache g++"
        log_info "Using ccache for faster rebuilds"
    fi
    
    # Set temporary directory on fastest storage
    for tmp_dir in "/tmp" "/dev/shm"; do
        if [[ -w "$tmp_dir" ]]; then
            export TMPDIR="$tmp_dir"
            log_debug "Using temporary directory: $tmp_dir"
            break
        fi
    done
    
    log_success "Build environment optimized"
}

# CMake configuration
configure_cmake() {
    log_info "Configuring CMake..."
    start_timer
    
    local cmake_args=(
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
        "-DCMAKE_VERBOSE_MAKEFILE=$([[ "$VERBOSE" == true ]] && echo ON || echo OFF)"
    )
    
    # Build-type specific flags
    case "$BUILD_TYPE" in
        "Release")
            cmake_args+=("-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=$([[ "$ENABLE_LTO" == true ]] && echo ON || echo OFF)")
            ;;
        "Debug")
            cmake_args+=("-DCMAKE_CXX_FLAGS_DEBUG=-g -O0 -DDEBUG -fsanitize=address")
            ;;
    esac
    
    # Optional features
    [[ "$ENABLE_TESTING" == true ]] && cmake_args+=("-DENABLE_TESTING=ON")
    [[ "$ENABLE_BENCHMARKS" == true ]] && cmake_args+=("-DENABLE_BENCHMARKS=ON")
    [[ "$ENABLE_STATIC_ANALYSIS" == true ]] && cmake_args+=("-DCMAKE_CXX_CLANG_TIDY=clang-tidy")
    
    # Run CMake
    cd "$BUILD_DIR"
    cmake "${cmake_args[@]}" "$PROJECT_DIR"
    
    end_timer
    log_success "CMake configuration completed"
}

# Build process
build_project() {
    log_info "Building project with $PARALLEL_JOBS parallel jobs..."
    start_timer
    
    local build_args=("-j$PARALLEL_JOBS")
    [[ "$VERBOSE" == true ]] && build_args+=("VERBOSE=1")
    
    # Build
    if [[ "$PROFILE_BUILD" == true ]]; then
        time make "${build_args[@]}"
    else
        make "${build_args[@]}"
    fi
    
    end_timer
    log_success "Build completed successfully"
}

# Testing
run_tests() {
    if [[ "$ENABLE_TESTING" == true ]]; then
        log_info "Running tests..."
        start_timer
        
        ctest --output-on-failure --parallel "$PARALLEL_JOBS"
        
        end_timer
        log_success "All tests passed"
    fi
}

# Installation
install_project() {
    if [[ "$INSTALL_TARGET" == true ]]; then
        log_info "Installing project..."
        start_timer
        
        sudo make install
        
        end_timer
        log_success "Installation completed"
    fi
}

# Package creation
create_package() {
    if [[ "$CREATE_PACKAGE" == true ]]; then
        log_info "Creating package..."
        start_timer
        
        make package
        
        end_timer
        log_success "Package created"
    fi
}

# Build summary
print_build_summary() {
    echo ""
    echo -e "${PURPLE}====== BUILD SUMMARY ======${NC}"
    echo -e "${BLUE}Project:${NC} Urology Inference Holoscan C++"
    echo -e "${BLUE}Build Type:${NC} $BUILD_TYPE"
    echo -e "${BLUE}Build Directory:${NC} $BUILD_DIR"
    echo -e "${BLUE}Parallel Jobs:${NC} $PARALLEL_JOBS"
    echo -e "${BLUE}LTO Enabled:${NC} $ENABLE_LTO"
    echo -e "${BLUE}ccache:${NC} $USE_CCACHE"
    echo ""
    
    # Find executables
    local executables=($(find "$BUILD_DIR" -name "urology_inference_holoscan_cpp*" -executable -type f))
    if [[ ${#executables[@]} -gt 0 ]]; then
        echo -e "${GREEN}Built executables:${NC}"
        for exe in "${executables[@]}"; do
            local size=$(du -h "$exe" | cut -f1)
            echo "  $exe ($size)"
        done
    fi
    
    echo ""
    echo -e "${GREEN}To run the application:${NC}"
    echo "  cd $BUILD_DIR"
    echo "  ./urology_inference_holoscan_cpp --help"
    echo ""
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug) BUILD_TYPE="Debug"; shift ;;
        --release) BUILD_TYPE="Release"; shift ;;
        --relwithdebinfo) BUILD_TYPE="RelWithDebInfo"; shift ;;
        --clean) CLEAN_BUILD=true; shift ;;
        --install-deps) INSTALL_DEPS=true; shift ;;
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --jobs|-j) PARALLEL_JOBS="$2"; shift 2 ;;
        --enable-testing) ENABLE_TESTING=true; shift ;;
        --enable-benchmarks) ENABLE_BENCHMARKS=true; shift ;;
        --enable-static-analysis) ENABLE_STATIC_ANALYSIS=true; shift ;;
        --disable-lto) ENABLE_LTO=false; shift ;;
        --disable-ccache) USE_CCACHE=false; shift ;;
        --verbose) VERBOSE=true; shift ;;
        --install) INSTALL_TARGET=true; shift ;;
        --package) CREATE_PACKAGE=true; shift ;;
        --profile) PROFILE_BUILD=true; shift ;;
        --help)
            echo "Optimized build script for Urology Inference Holoscan C++"
            echo ""
            echo "Usage: $0 [options]"
            echo ""
            echo "Build Options:"
            echo "  --debug                   Build in debug mode"
            echo "  --release                 Build in release mode (default)"
            echo "  --relwithdebinfo          Build with debug info and optimizations"
            echo "  --clean                   Clean build directory before building"
            echo "  --build-dir DIR           Specify build directory (default: build)"
            echo ""
            echo "Performance Options:"
            echo "  --jobs,-j N               Number of parallel jobs (default: auto)"
            echo "  --disable-lto             Disable Link Time Optimization"
            echo "  --disable-ccache          Disable compiler cache"
            echo "  --profile                 Profile build time"
            echo ""
            echo "Feature Options:"
            echo "  --enable-testing          Enable unit tests"
            echo "  --enable-benchmarks       Enable performance benchmarks"
            echo "  --enable-static-analysis  Enable static code analysis"
            echo ""
            echo "Other Options:"
            echo "  --install-deps            Install dependencies"
            echo "  --install                 Install the built application"
            echo "  --package                 Create installation package"
            echo "  --verbose                 Enable verbose output"
            echo "  --help                    Show this help message"
            exit 0
            ;;
        *) log_error "Unknown option: $1"; exit 1 ;;
    esac
done

# Main execution
echo -e "${PURPLE}======================================${NC}"
echo -e "${PURPLE}  Urology Inference Holoscan C++${NC}"
echo -e "${PURPLE}     Optimized Build Script${NC}"
echo -e "${PURPLE}======================================${NC}"
echo ""

# Dependency installation
if [[ "$INSTALL_DEPS" == true ]]; then
    log_info "Installing dependencies..."
    "$SCRIPT_DIR/install_video_encoder_deps.sh"
fi

# System checks
check_dependencies
check_holoscan_dependencies
optimize_build_environment

# Clean build if requested
if [[ "$CLEAN_BUILD" == true ]]; then
    log_info "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
BUILD_DIR="$(cd "$BUILD_DIR" && pwd)"  # Get absolute path

# Build process
configure_cmake
build_project
run_tests
install_project
create_package

# Summary
print_build_summary

log_success "Build process completed successfully!" 