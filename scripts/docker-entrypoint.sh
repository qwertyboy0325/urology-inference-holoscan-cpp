#!/bin/bash
set -e

# Docker entrypoint script for Urology Inference Holoscan C++
# This script handles initialization and command execution in the container

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Function to check video encoder dependencies
check_video_encoder_deps() {
    log_info "Checking video encoder dependencies..."
    
    local holoscan_libs_dir="/opt/nvidia/holoscan/lib/"
    local required_libs=("libgxf_encoder.so" "libgxf_encoderio.so" "libgxf_decoder.so" "libgxf_decoderio.so")
    local missing_libs=()
    
    # Check if Holoscan libraries directory exists
    if [[ ! -d "$holoscan_libs_dir" ]]; then
        log_error "Holoscan libraries directory not found: $holoscan_libs_dir"
        return 1
    fi
    
    # Check each required library
    for lib in "${required_libs[@]}"; do
        if [[ -f "$holoscan_libs_dir/$lib" ]]; then
            # Check if library is readable and not empty
            if [[ -r "$holoscan_libs_dir/$lib" ]] && [[ -s "$holoscan_libs_dir/$lib" ]]; then
                log_info "✅ Found $lib"
            else
                log_warning "⚠️  $lib exists but may be invalid"
                missing_libs+=("$lib")
            fi
        else
            log_warning "❌ Missing $lib"
            missing_libs+=("$lib")
        fi
    done
    
    # Report results
    if [[ ${#missing_libs[@]} -eq 0 ]]; then
        log_success "All video encoder dependencies are available"
        return 0
    else
        log_warning "Some video encoder dependencies are missing or invalid:"
        for lib in "${missing_libs[@]}"; do
            log_warning "  - $lib"
        done
        log_warning "Video encoding features may not work properly"
        log_warning "You can verify/reinstall dependencies by running:"
        log_warning "  ./scripts/install_video_encoder_deps.sh"
        return 0  # Don't fail container startup, just warn
    fi
}

# Detailed verification function (similar to the one in install script)
verify_video_encoder_deps_detailed() {
    echo ""
    echo -e "${BLUE}=== Comprehensive Video Encoder Dependencies Verification ===${NC}"
    
    local verification_failed=0
    local holoscan_libs_dir="/opt/nvidia/holoscan/lib/"
    local required_libs=("libgxf_encoder.so" "libgxf_encoderio.so" "libgxf_decoder.so" "libgxf_decoderio.so")
    local arch=$(arch)
    
    # Check if Holoscan libraries directory exists
    if [[ ! -d "$holoscan_libs_dir" ]]; then
        log_error "Holoscan libraries directory not found: $holoscan_libs_dir"
        log_error "Please ensure Holoscan SDK is properly installed."
        return 1
    fi
    
    log_info "📁 Checking libraries in: $holoscan_libs_dir"
    
    # Verify each required library
    for lib in "${required_libs[@]}"; do
        if [[ -f "$holoscan_libs_dir/$lib" ]]; then
            # Check if library is readable and has correct permissions
            if [[ -r "$holoscan_libs_dir/$lib" ]]; then
                local lib_size=$(stat -c%s "$holoscan_libs_dir/$lib" 2>/dev/null || echo "unknown")
                if [[ "$lib_size" != "0" ]] && [[ "$lib_size" != "unknown" ]]; then
                    log_success "✅ $lib (size: $lib_size bytes)"
                else
                    log_error "❌ $lib (empty or unreadable file)"
                    verification_failed=1
                fi
            else
                log_error "❌ $lib (not readable)"
                verification_failed=1
            fi
        else
            log_error "❌ $lib (not found)"
            verification_failed=1
        fi
    done
    
    # Check library dependencies using ldd (if available)
    echo ""
    log_info "📚 Checking library dependencies..."
    for lib in "${required_libs[@]}"; do
        if [[ -f "$holoscan_libs_dir/$lib" ]]; then
            if command -v ldd &> /dev/null; then
                echo "Dependencies for $lib:"
                if ldd "$holoscan_libs_dir/$lib" 2>/dev/null | grep -q "not found"; then
                    log_warning "⚠️  Warning: Missing dependencies for $lib"
                    ldd "$holoscan_libs_dir/$lib" 2>/dev/null | grep "not found" || true
                    verification_failed=1
                else
                    log_success "✅ All dependencies satisfied for $lib"
                fi
            else
                log_warning "⚠️  ldd not available, skipping dependency check"
            fi
        fi
    done
    
    # Check DeepStream dependencies for x86_64
    if [[ "$arch" == "x86_64" ]]; then
        echo ""
        log_info "🔍 Checking DeepStream dependencies..."
        if dpkg -l 2>/dev/null | grep -q nvv4l2; then
            log_success "✅ DeepStream nvv4l2 package installed"
        else
            log_warning "⚠️  DeepStream nvv4l2 package not found"
            log_warning "   This may not be critical if you don't use DeepStream features"
        fi
    fi
    
    # Check GXF extension registration
    echo ""
    log_info "🔧 Checking GXF extension compatibility..."
    local gxf_registry_path="/opt/nvidia/holoscan/lib/gxf_extensions"
    if [[ -d "$gxf_registry_path" ]]; then
        log_success "✅ GXF extensions directory found: $gxf_registry_path"
    else
        log_warning "⚠️  GXF extensions directory not found: $gxf_registry_path"
        log_warning "   Extensions may still work if loaded directly"
    fi
    
    # Summary
    echo ""
    echo -e "${BLUE}=== Verification Summary ===${NC}"
    if [[ $verification_failed -eq 0 ]]; then
        log_success "✅ All video encoder dependencies verified successfully!"
        echo ""
        echo -e "${GREEN}📋 Verified components:${NC}"
        echo "  - GXF Encoder Extension"
        echo "  - GXF EncoderIO Extension" 
        echo "  - GXF Decoder Extension"
        echo "  - GXF DecoderIO Extension"
        if [[ "$arch" == "x86_64" ]]; then
            echo "  - DeepStream V4L2 Dependencies"
        fi
        echo ""
        log_success "🚀 Your Holoscan application can use video encoding/decoding features!"
        return 0
    else
        log_error "❌ Verification failed! Some dependencies are missing or invalid."
        echo ""
        echo -e "${YELLOW}🔧 Troubleshooting steps:${NC}"
        echo "  1. Ensure container was built with video encoder support"
        echo "  2. Check if the build process completed successfully"
        echo "  3. Verify NVIDIA GPU drivers are available (nvidia-smi)"
        echo "  4. Check Holoscan SDK installation"
        echo ""
        echo -e "${YELLOW}📞 If issues persist, check:${NC}"
        echo "  - Holoscan documentation: https://docs.nvidia.com/holoscan/"
        echo "  - Rebuild container with: docker build --no-cache"
        return 1
    fi
}

# Function to check environment
check_environment() {
    log_info "Checking container environment..."
    
    # Check NVIDIA GPU availability
    if command -v nvidia-smi &> /dev/null; then
        log_info "NVIDIA GPU driver detected:"
        nvidia-smi --query-gpu=name,memory.total,memory.used --format=csv,noheader,nounits | head -1
    else
        log_warning "No NVIDIA GPU driver detected. Running in CPU-only mode."
    fi
    
    # Check CUDA availability
    if command -v nvcc &> /dev/null; then
        CUDA_VERSION=$(nvcc --version | grep "release" | sed 's/.*release \([0-9]\+\.[0-9]\+\).*/\1/')
        log_info "CUDA version: $CUDA_VERSION"
    else
        log_warning "CUDA not available."
    fi
    
    # Check Holoscan SDK
    if pkg-config --exists holoscan; then
        HOLOSCAN_VERSION=$(pkg-config --modversion holoscan)
        log_info "Holoscan SDK version: $HOLOSCAN_VERSION"
    else
        log_error "Holoscan SDK not found!"
        exit 1
    fi
    
    # Check video encoder dependencies
    check_video_encoder_deps
    
    log_success "Environment check completed"
}

# Function to initialize application directories
initialize_directories() {
    log_info "Initializing application directories..."
    
    # Create required directories if they don't exist
    mkdir -p "${UROLOGY_LOG_PATH}" "${UROLOGY_OUTPUT_PATH}" "${HOLOSCAN_MODEL_PATH}"
    
    # Set appropriate permissions
    chmod 755 "${UROLOGY_LOG_PATH}" "${UROLOGY_OUTPUT_PATH}" "${HOLOSCAN_MODEL_PATH}"
    
    log_success "Directories initialized"
}

# Function to validate configuration
validate_configuration() {
    log_info "Validating configuration..."
    
    # Check if config files exist
    if [ ! -d "${UROLOGY_CONFIG_PATH}" ]; then
        log_error "Configuration directory not found: ${UROLOGY_CONFIG_PATH}"
        exit 1
    fi
    
    # Check for required config files
    local config_files=("app_config.yaml" "labels.yaml")
    for file in "${config_files[@]}"; do
        if [ ! -f "${UROLOGY_CONFIG_PATH}/${file}" ]; then
            log_warning "Config file not found: ${file}"
        else
            log_info "Found config file: ${file}"
        fi
    done
    
    log_success "Configuration validation completed"
}

# Function to print application info
print_application_info() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  Urology Inference Holoscan C++${NC}"
    echo -e "${BLUE}     Docker Container${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
    echo -e "${GREEN}Environment Variables:${NC}"
    echo "  HOLOHUB_DATA_PATH:    ${HOLOHUB_DATA_PATH}"
    echo "  HOLOSCAN_MODEL_PATH:  ${HOLOSCAN_MODEL_PATH}"
    echo "  UROLOGY_CONFIG_PATH:  ${UROLOGY_CONFIG_PATH}"
    echo "  UROLOGY_LOG_PATH:     ${UROLOGY_LOG_PATH}"
    echo "  UROLOGY_OUTPUT_PATH:  ${UROLOGY_OUTPUT_PATH}"
    echo ""
    echo -e "${GREEN}Available Commands:${NC}"
    echo "  ./urology_inference_holoscan_cpp --help    Show application help"
    echo "  ./tests/unit_tests                         Run unit tests"
    echo "  ./benchmarks/performance_benchmarks        Run performance benchmarks"
    echo ""
}

# Function to handle special commands
handle_special_commands() {
    case "$1" in
        "test"|"tests")
            log_info "Running unit tests..."
            exec ./tests/unit_tests "${@:2}"
            ;;
        "benchmark"|"benchmarks")
            log_info "Running performance benchmarks..."
            exec ./benchmarks/performance_benchmarks "${@:2}"
            ;;
        "shell"|"bash")
            log_info "Starting interactive shell..."
            exec /bin/bash "${@:2}"
            ;;
        "version"|"--version")
            echo "Urology Inference Holoscan C++ - Docker Version 1.0.0"
            echo "Based on NVIDIA Holoscan SDK 3.3.0"
            exit 0
            ;;
        "env"|"environment")
            check_environment
            print_application_info
            exit 0
            ;;
        "verify-deps"|"verify-dependencies")
            log_info "Running comprehensive dependency verification..."
            if [[ -f "./scripts/verify_video_encoder_deps.sh" ]]; then
                ./scripts/verify_video_encoder_deps.sh --verbose
            else
                log_warning "Standalone verification script not found, using built-in verification..."
                verify_video_encoder_deps_detailed
            fi
            exit $?
            ;;
        "help"|"--help"|"-h")
            echo "Docker Container Commands:"
            echo "  test          Run unit tests"
            echo "  benchmark     Run performance benchmarks"
            echo "  shell         Start interactive shell"
            echo "  version       Show version information"
            echo "  env           Show environment information"
            echo "  verify-deps   Verify video encoder dependencies"
            echo "  help          Show this help"
            echo ""
            echo "Application Help:"
            ./urology_inference_holoscan_cpp --help
            exit 0
            ;;
    esac
}

# Function to setup logging
setup_logging() {
    # Create log directory if it doesn't exist
    mkdir -p "${UROLOGY_LOG_PATH}"
    
    # Setup log file with timestamp
    LOG_FILE="${UROLOGY_LOG_PATH}/urology_$(date +%Y%m%d_%H%M%S).log"
    export UROLOGY_LOG_FILE="${LOG_FILE}"
    
    log_info "Logging to: ${LOG_FILE}"
}

# Main execution
main() {
    # Print application info
    print_application_info
    
    # Check environment
    check_environment
    
    # Initialize directories
    initialize_directories
    
    # Setup logging
    setup_logging
    
    # Validate configuration
    validate_configuration
    
    # Handle special commands
    if [ $# -gt 0 ]; then
        handle_special_commands "$@"
    fi
    
    # If no special command, run the application
    log_info "Starting Urology Inference application..."
    
    # Execute the main application with all passed arguments
    exec ./urology_inference_holoscan_cpp "$@"
}

# Trap signals for graceful shutdown
trap 'log_info "Received shutdown signal, cleaning up..."; exit 0' SIGTERM SIGINT

# Run main function
main "$@" 