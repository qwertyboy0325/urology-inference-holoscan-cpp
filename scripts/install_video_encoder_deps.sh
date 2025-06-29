#!/bin/bash

# SPDX-FileCopyrightText: Copyright (c) 2023-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

# Install video encoder dependencies for Holoscan C++ application
# Based on holohub install_dependencies.sh

set -e

echo "Installing video encoder dependencies for Holoscan C++ application..."

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   echo "This script should not be run as root"
   exit 1
fi

# Get architecture
ARCH=$(arch)
HOLOSCAN_LIBS_DIR="/opt/nvidia/holoscan/lib/"

# Check architecture support
if [[ $ARCH != "x86_64" ]] && [[ $ARCH != "aarch64" ]]; then
    echo "Unsupported architecture: $ARCH"
    echo "Only x86_64 and aarch64 are supported"
    exit 1
fi

echo "Detected architecture: $ARCH"

# Create temporary directory for downloads
TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"

echo "Downloading GXF multimedia extensions..."

# Define GXF multimedia extensions
declare -A gxf_mm_extensions
gxf_mm_extensions=(
    ["encoder"]="ea5c44e4-15db-4448-a3a6-f32004303338"
    ["encoderio"]="1a0a562d-378c-4618-bb5e-d3f70825aed2"
    ["decoder"]="edc99001-73bd-435c-af0c-e013dcda3439"
    ["decoderio"]="45081ccb-982e-4946-96f9-0d684f2cfbd0"
)

# Download and install extensions
for extension_name in "${!gxf_mm_extensions[@]}"; do
    extension_id="${gxf_mm_extensions[$extension_name]}"
    
    echo "Downloading gxf_$extension_name extension..."
    
    # Download extension
    wget -q "https://api.ngc.nvidia.com/v2/resources/org/nvidia/gxf/gxf_$extension_name/versions/2.4.0/files/gxf_$extension_name-2.4.0-$ARCH.tar.gz" \
         -O "gxf_$extension_name.tar.gz" || {
        echo "Failed to download gxf_$extension_name extension"
        continue
    }
    
    # Extract extension
    tar -xzf "gxf_$extension_name.tar.gz"
    
    # Install extension
    if [[ -f "gxf_$extension_name/libgxf_$extension_name.so" ]]; then
        sudo cp "gxf_$extension_name/libgxf_$extension_name.so" "$HOLOSCAN_LIBS_DIR"
        echo "Installed libgxf_$extension_name.so"
    else
        echo "Warning: libgxf_$extension_name.so not found in archive"
    fi
done

# Download and install DeepStream dependencies if needed
if [[ $ARCH == "x86_64" ]]; then
    echo "Downloading DeepStream dependencies for x86_64..."
    
    DS_DEPS_URL="https://api.ngc.nvidia.com/v2/resources/org/nvidia/gxf_and_gc/4.0.0/files?redirect=true&path=nvv4l2_x86_ds-7.0.deb"
    
    wget -q "$DS_DEPS_URL" -O "nvv4l2_x86_ds-7.0.deb" || {
        echo "Warning: Failed to download DeepStream dependencies"
    }
    
    if [[ -f "nvv4l2_x86_ds-7.0.deb" ]]; then
        sudo dpkg -i "nvv4l2_x86_ds-7.0.deb" || {
            echo "Warning: Failed to install DeepStream dependencies"
            echo "You may need to run: sudo apt-get install -f"
        }
    fi
fi

# Cleanup
cd /
rm -rf "$TEMP_DIR"

echo "Installation completed!"
echo ""

# Verification function
verify_installation() {
    echo "=== Verifying Video Encoder Dependencies Installation ==="
    
    local verification_failed=0
    local required_libs=("libgxf_encoder.so" "libgxf_encoderio.so" "libgxf_decoder.so" "libgxf_decoderio.so")
    
    # Check if Holoscan libraries directory exists
    if [[ ! -d "$HOLOSCAN_LIBS_DIR" ]]; then
        echo "❌ ERROR: Holoscan libraries directory not found: $HOLOSCAN_LIBS_DIR"
        echo "   Please ensure Holoscan SDK is properly installed."
        return 1
    fi
    
    echo "📁 Checking libraries in: $HOLOSCAN_LIBS_DIR"
    
    # Verify each required library
    for lib in "${required_libs[@]}"; do
        if [[ -f "$HOLOSCAN_LIBS_DIR/$lib" ]]; then
            # Check if library is readable and has correct permissions
            if [[ -r "$HOLOSCAN_LIBS_DIR/$lib" ]]; then
                local lib_size=$(stat -f%z "$HOLOSCAN_LIBS_DIR/$lib" 2>/dev/null || stat -c%s "$HOLOSCAN_LIBS_DIR/$lib" 2>/dev/null)
                if [[ $lib_size -gt 0 ]]; then
                    echo "✅ $lib (size: $lib_size bytes)"
                else
                    echo "❌ $lib (empty file)"
                    verification_failed=1
                fi
            else
                echo "❌ $lib (not readable)"
                verification_failed=1
            fi
        else
            echo "❌ $lib (not found)"
            verification_failed=1
        fi
    done
    
    # Check library dependencies using ldd (if available)
    echo ""
    echo "📚 Checking library dependencies..."
    for lib in "${required_libs[@]}"; do
        if [[ -f "$HOLOSCAN_LIBS_DIR/$lib" ]]; then
            if command -v ldd &> /dev/null; then
                echo "Dependencies for $lib:"
                if ldd "$HOLOSCAN_LIBS_DIR/$lib" | grep -q "not found"; then
                    echo "⚠️  Warning: Missing dependencies for $lib"
                    ldd "$HOLOSCAN_LIBS_DIR/$lib" | grep "not found"
                    verification_failed=1
                else
                    echo "✅ All dependencies satisfied for $lib"
                fi
            else
                echo "⚠️  ldd not available, skipping dependency check"
            fi
        fi
    done
    
    # Check DeepStream dependencies for x86_64
    if [[ $ARCH == "x86_64" ]]; then
        echo ""
        echo "🔍 Checking DeepStream dependencies..."
        if dpkg -l | grep -q nvv4l2; then
            echo "✅ DeepStream nvv4l2 package installed"
        else
            echo "⚠️  DeepStream nvv4l2 package not found"
            echo "   This may not be critical if you don't use DeepStream features"
        fi
    fi
    
    # Check GXF extension registration
    echo ""
    echo "🔧 Checking GXF extension compatibility..."
    local gxf_registry_path="/opt/nvidia/holoscan/lib/gxf_extensions"
    if [[ -d "$gxf_registry_path" ]]; then
        echo "✅ GXF extensions directory found: $gxf_registry_path"
    else
        echo "⚠️  GXF extensions directory not found: $gxf_registry_path"
        echo "   Extensions may still work if loaded directly"
    fi
    
    # Summary
    echo ""
    echo "=== Verification Summary ==="
    if [[ $verification_failed -eq 0 ]]; then
        echo "✅ All video encoder dependencies verified successfully!"
        echo ""
        echo "📋 Installed components:"
        echo "  - GXF Encoder Extension"
        echo "  - GXF EncoderIO Extension" 
        echo "  - GXF Decoder Extension"
        echo "  - GXF DecoderIO Extension"
        if [[ $ARCH == "x86_64" ]]; then
            echo "  - DeepStream V4L2 Dependencies"
        fi
        echo ""
        echo "🚀 Your Holoscan application can now use video encoding/decoding features!"
        return 0
    else
        echo "❌ Verification failed! Some dependencies are missing or invalid."
        echo ""
        echo "🔧 Troubleshooting steps:"
        echo "  1. Ensure NVIDIA GPU drivers are installed (nvidia-smi)"
        echo "  2. Verify CUDA is properly configured (nvcc --version)"
        echo "  3. Check Holoscan SDK installation (/opt/nvidia/holoscan/)"
        echo "  4. Verify write permissions to system directories"
        echo "  5. Check network connectivity for downloads"
        echo "  6. Try running the script with sudo if permission issues persist"
        echo ""
        echo "📞 If issues persist, check:"
        echo "  - Holoscan documentation: https://docs.nvidia.com/holoscan/"
        echo "  - NGC container registry: https://catalog.ngc.nvidia.com/"
        return 1
    fi
}

# Run verification
if verify_installation; then
    echo ""
    echo "🎉 Installation and verification completed successfully!"
    exit 0
else
    echo ""
    echo "💥 Installation completed but verification failed!"
    echo "Please check the issues above before using video encoder features."
    exit 1
fi 