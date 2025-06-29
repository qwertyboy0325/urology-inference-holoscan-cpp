#!/bin/bash

# Docker build script for Urology Inference Holoscan C++
# Supports building different stages with optimizations

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

# Default values
BUILD_TARGET="runtime"
BUILD_TYPE="Release"
PUSH_IMAGE=false
NO_CACHE=false
VERBOSE=false
IMAGE_TAG=""

# Functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

show_help() {
    echo "Docker build script for Urology Inference Holoscan C++"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Build Targets:"
    echo "  --runtime             Build runtime image (default)"
    echo "  --development         Build development image"
    echo "  --all                 Build all images"
    echo ""
    echo "Build Options:"
    echo "  --debug               Build in debug mode"
    echo "  --release             Build in release mode (default)"
    echo "  --no-cache            Build without using cache"
    echo "  --push                Push image to registry after build"
    echo "  --tag TAG             Set custom image tag"
    echo "  --verbose             Enable verbose output"
    echo ""
    echo "Examples:"
    echo "  $0 --runtime --release          # Build optimized runtime image"
    echo "  $0 --development --debug        # Build development image with debug"
    echo "  $0 --all --tag v1.0.0           # Build all images with custom tag"
    echo "  $0 --runtime --push             # Build and push runtime image"
    echo ""
}

check_prerequisites() {
    log_info "Checking prerequisites..."
    
    # Check Docker
    if ! command -v docker &> /dev/null; then
        log_error "Docker is not installed or not in PATH"
        exit 1
    fi
    
    # Check Docker Buildx
    if ! docker buildx version &> /dev/null; then
        log_error "Docker Buildx is not available"
        exit 1
    fi
    
    # Check NVIDIA Docker runtime
    if ! docker info | grep -q nvidia; then
        log_warning "NVIDIA Docker runtime not detected"
        log_warning "GPU features may not be available"
    fi
    
    log_success "Prerequisites check completed"
}

get_image_tag() {
    if [[ -n "$IMAGE_TAG" ]]; then
        echo "$IMAGE_TAG"
        return
    fi
    
    # Generate tag based on target and build type
    local tag="${BUILD_TARGET}-${BUILD_TYPE,,}"
    
    # Add git commit hash if available
    if command -v git &> /dev/null && git rev-parse --git-dir &> /dev/null; then
        local git_hash=$(git rev-parse --short HEAD)
        tag="${tag}-${git_hash}"
    fi
    
    echo "$tag"
}

build_image() {
    local target=$1
    local tag=$(get_image_tag)
    
    log_info "Building $target image with tag: $tag"
    
    # Build arguments
    local build_args=(
        "--target" "$target"
        "--tag" "urology-inference:$tag"
        "--build-arg" "BUILDKIT_INLINE_CACHE=1"
        "--build-arg" "CMAKE_BUILD_TYPE=$BUILD_TYPE"
    )
    
    # Optional arguments
    [[ "$NO_CACHE" == true ]] && build_args+=("--no-cache")
    [[ "$VERBOSE" == true ]] && build_args+=("--progress=plain")
    
    # Use buildx for better caching and multi-platform support
    docker buildx build "${build_args[@]}" .
    
    # Tag with latest for convenience
    docker tag "urology-inference:$tag" "urology-inference:$target-latest"
    
    log_success "Built image: urology-inference:$tag"
}

push_image() {
    local tag=$(get_image_tag)
    
    if [[ "$PUSH_IMAGE" == true ]]; then
        log_info "Pushing image to registry..."
        
        # Push specific tag
        docker push "urology-inference:$tag"
        
        # Push latest tag
        docker push "urology-inference:$BUILD_TARGET-latest"
        
        log_success "Image pushed successfully"
    fi
}

print_build_summary() {
    local tag=$(get_image_tag)
    
    echo ""
    echo -e "${PURPLE}====== BUILD SUMMARY ======${NC}"
    echo -e "${BLUE}Target:${NC} $BUILD_TARGET"
    echo -e "${BLUE}Build Type:${NC} $BUILD_TYPE"
    echo -e "${BLUE}Image Tag:${NC} $tag"
    echo ""
    
    # Show built images
    echo -e "${GREEN}Built Images:${NC}"
    docker images "urology-inference" --format "table {{.Repository}}:{{.Tag}}\t{{.Size}}\t{{.CreatedAt}}"
    
    echo ""
    echo -e "${GREEN}Usage Examples:${NC}"
    echo "  # Run the application"
    echo "  docker run --gpus all urology-inference:$tag --help"
    echo ""
    echo "  # Use with Docker Compose"
    echo "  docker-compose up urology-inference"
    echo ""
    echo "  # Development mode"
    if [[ "$BUILD_TARGET" == "development" ]]; then
        echo "  docker run --gpus all -it urology-inference:$tag"
    else
        echo "  docker-compose --profile development up urology-inference-dev"
    fi
    echo ""
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --runtime)
            BUILD_TARGET="runtime"
            shift
            ;;
        --development)
            BUILD_TARGET="development"
            shift
            ;;
        --all)
            BUILD_TARGET="all"
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --no-cache)
            NO_CACHE=true
            shift
            ;;
        --push)
            PUSH_IMAGE=true
            shift
            ;;
        --tag)
            IMAGE_TAG="$2"
            shift 2
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --help|-h)
            show_help
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Main execution
echo -e "${PURPLE}======================================${NC}"
echo -e "${PURPLE}  Urology Inference Docker Build${NC}"
echo -e "${PURPLE}    Based on Holoscan SDK 3.3.0${NC}"
echo -e "${PURPLE}======================================${NC}"
echo ""

# Check prerequisites
check_prerequisites

# Build based on target
case "$BUILD_TARGET" in
    "runtime")
        build_image "runtime"
        push_image
        ;;
    "development")
        build_image "development"
        push_image
        ;;
    "all")
        log_info "Building all images..."
        
        # Build runtime image
        BUILD_TARGET="runtime"
        build_image "runtime"
        
        # Build development image
        BUILD_TARGET="development"
        build_image "development"
        
        # Push if requested
        if [[ "$PUSH_IMAGE" == true ]]; then
            BUILD_TARGET="runtime"
            push_image
            BUILD_TARGET="development"
            push_image
        fi
        
        BUILD_TARGET="all"
        ;;
    *)
        log_error "Invalid build target: $BUILD_TARGET"
        exit 1
        ;;
esac

# Print summary
print_build_summary

log_success "Docker build completed successfully!" 