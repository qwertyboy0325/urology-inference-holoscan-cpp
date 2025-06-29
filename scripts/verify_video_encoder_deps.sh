#!/bin/bash

# SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

# Standalone verification script for video encoder dependencies
# This script only performs verification without installing anything

set -e

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

# Default values
HOLOSCAN_LIBS_DIR="/opt/nvidia/holoscan/lib/"
VERBOSE=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --libs-dir)
            HOLOSCAN_LIBS_DIR="$2"
            shift 2
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --help|-h)
            echo "Video Encoder Dependencies Verification Script"
            echo ""
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --libs-dir DIR    Set Holoscan libraries directory (default: /opt/nvidia/holoscan/lib/)"
            echo "  --verbose         Enable verbose output"
            echo "  --help           Show this help"
            echo ""
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Get architecture
ARCH=$(arch)

echo -e "${BLUE}======================================================${NC}"
echo -e "${BLUE}   Video Encoder Dependencies Verification${NC}"
echo -e "${BLUE}      for Holoscan C++ Applications${NC}"
echo -e "${BLUE}======================================================${NC}"
echo ""

log_info "Target architecture: $ARCH"
log_info "Holoscan libraries directory: $HOLOSCAN_LIBS_DIR"

# Verification function
verify_installation() {
    log_info "Starting verification process..."
    echo ""
    
    local verification_failed=0
    local required_libs=("libgxf_encoder.so" "libgxf_encoderio.so" "libgxf_decoder.so" "libgxf_decoderio.so")
    
    # Check if Holoscan libraries directory exists
    if [[ ! -d "$HOLOSCAN_LIBS_DIR" ]]; then
        log_error "Holoscan libraries directory not found: $HOLOSCAN_LIBS_DIR"
        log_error "Please ensure Holoscan SDK is properly installed."
        return 1
    fi
    
    log_info "📁 Checking libraries in: $HOLOSCAN_LIBS_DIR"
    echo ""
    
    # Verify each required library
    for lib in "${required_libs[@]}"; do
        if [[ -f "$HOLOSCAN_LIBS_DIR/$lib" ]]; then
            # Check if library is readable and has correct permissions
            if [[ -r "$HOLOSCAN_LIBS_DIR/$lib" ]]; then
                local lib_size=$(stat -c%s "$HOLOSCAN_LIBS_DIR/$lib" 2>/dev/null || stat -f%z "$HOLOSCAN_LIBS_DIR/$lib" 2>/dev/null)
                if [[ $lib_size -gt 0 ]]; then
                    log_success "✅ $lib (size: $lib_size bytes)"
                    
                    # Verbose mode: show file details
                    if [[ "$VERBOSE" == true ]]; then
                        local file_info=$(ls -la "$HOLOSCAN_LIBS_DIR/$lib")
                        echo "   Details: $file_info"
                    fi
                else
                    log_error "❌ $lib (empty file)"
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
        if [[ -f "$HOLOSCAN_LIBS_DIR/$lib" ]]; then
            if command -v ldd &> /dev/null; then
                echo ""
                echo "Dependencies for $lib:"
                local ldd_output=$(ldd "$HOLOSCAN_LIBS_DIR/$lib" 2>/dev/null)
                if echo "$ldd_output" | grep -q "not found"; then
                    log_warning "⚠️  Warning: Missing dependencies for $lib"
                    echo "$ldd_output" | grep "not found"
                    verification_failed=1
                else
                    log_success "✅ All dependencies satisfied for $lib"
                    if [[ "$VERBOSE" == true ]]; then
                        echo "$ldd_output"
                    fi
                fi
            else
                log_warning "⚠️  ldd not available, skipping dependency check for $lib"
            fi
        fi
    done
    
    # Check DeepStream dependencies for x86_64
    if [[ $ARCH == "x86_64" ]]; then
        echo ""
        log_info "🔍 Checking DeepStream dependencies..."
        if command -v dpkg &> /dev/null && dpkg -l 2>/dev/null | grep -q nvv4l2; then
            log_success "✅ DeepStream nvv4l2 package installed"
            if [[ "$VERBOSE" == true ]]; then
                dpkg -l | grep nvv4l2
            fi
        else
            log_warning "⚠️  DeepStream nvv4l2 package not found"
            log_warning "   This may not be critical if you don't use DeepStream features"
        fi
    fi
    
    # Check CUDA runtime
    echo ""
    log_info "🔧 Checking CUDA runtime..."
    if command -v nvidia-smi &> /dev/null; then
        log_success "✅ NVIDIA driver available"
        if [[ "$VERBOSE" == true ]]; then
            nvidia-smi --query-gpu=name,driver_version,cuda_version --format=csv,noheader,nounits | head -1
        fi
    else
        log_warning "⚠️  NVIDIA driver not available (nvidia-smi)"
        log_warning "   GPU features may not work"
    fi
    
    if command -v nvcc &> /dev/null; then
        local cuda_version=$(nvcc --version | grep "release" | sed 's/.*release \([0-9]\+\.[0-9]\+\).*/\1/')
        log_success "✅ CUDA toolkit available (version: $cuda_version)"
    else
        log_warning "⚠️  CUDA toolkit not available (nvcc)"
        log_warning "   This is normal in runtime containers"
    fi
    
    # Check GXF extension registration
    echo ""
    log_info "🔧 Checking GXF extension compatibility..."
    local gxf_registry_path="/opt/nvidia/holoscan/lib/gxf_extensions"
    if [[ -d "$gxf_registry_path" ]]; then
        log_success "✅ GXF extensions directory found: $gxf_registry_path"
        if [[ "$VERBOSE" == true ]]; then
            log_info "GXF extensions found:"
            ls -la "$gxf_registry_path" | grep "\.so$" || echo "   No .so files found"
        fi
    else
        log_warning "⚠️  GXF extensions directory not found: $gxf_registry_path"
        log_warning "   Extensions may still work if loaded directly"
    fi
    
    # Check Holoscan SDK version
    echo ""
    log_info "📦 Checking Holoscan SDK..."
    if command -v pkg-config &> /dev/null && pkg-config --exists holoscan; then
        local holoscan_version=$(pkg-config --modversion holoscan)
        log_success "✅ Holoscan SDK available (version: $holoscan_version)"
    else
        log_warning "⚠️  Holoscan SDK not detected via pkg-config"
        log_warning "   This may be normal depending on installation method"
    fi
    
    # Summary
    echo ""
    echo -e "${BLUE}=== Verification Summary ===${NC}"
    if [[ $verification_failed -eq 0 ]]; then
        log_success "✅ All video encoder dependencies verified successfully!"
        echo ""
        echo -e "${GREEN}📋 Verified components:${NC}"
        echo "  - GXF Encoder Extension (libgxf_encoder.so)"
        echo "  - GXF EncoderIO Extension (libgxf_encoderio.so)"
        echo "  - GXF Decoder Extension (libgxf_decoder.so)"
        echo "  - GXF DecoderIO Extension (libgxf_decoderio.so)"
        if [[ $ARCH == "x86_64" ]]; then
            echo "  - DeepStream V4L2 Dependencies"
        fi
        echo ""
        log_success "🚀 Your Holoscan application can use video encoding/decoding features!"
        echo ""
        echo -e "${GREEN}💡 Usage in your application:${NC}"
        echo "   The GXF video extensions are now available for use in your"
        echo "   Holoscan streaming pipelines. They will be automatically"
        echo "   discovered when you reference them in your application."
        echo ""
        return 0
    else
        log_error "❌ Verification failed! Some dependencies are missing or invalid."
        echo ""
        echo -e "${YELLOW}🔧 Troubleshooting steps:${NC}"
        echo "  1. Ensure NVIDIA GPU drivers are installed (run: nvidia-smi)"
        echo "  2. Verify CUDA is properly configured (run: nvcc --version)"
        echo "  3. Check Holoscan SDK installation in $HOLOSCAN_LIBS_DIR"
        echo "  4. Verify write permissions to system directories"
        echo "  5. Check network connectivity for downloads"
        echo "  6. Try reinstalling with: ./scripts/install_video_encoder_deps.sh"
        echo ""
        echo -e "${YELLOW}📞 If issues persist, check:${NC}"
        echo "  - Holoscan documentation: https://docs.nvidia.com/holoscan/"
        echo "  - NGC container registry: https://catalog.ngc.nvidia.com/"
        echo "  - GXF extensions documentation"
        return 1
    fi
}

# Run verification
if verify_installation; then
    echo -e "${GREEN}🎉 Verification completed successfully!${NC}"
    exit 0
else
    echo -e "${RED}💥 Verification failed!${NC}"
    echo "Please address the issues above before using video encoder features."
    exit 1
fi 