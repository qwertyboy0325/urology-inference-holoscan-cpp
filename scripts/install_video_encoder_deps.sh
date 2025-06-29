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
echo "Installed GXF extensions:"
echo "  - libgxf_encoder.so"
echo "  - libgxf_encoderio.so"
echo "  - libgxf_decoder.so"
echo "  - libgxf_decoderio.so"
echo ""
echo "Extensions installed to: $HOLOSCAN_LIBS_DIR"
echo ""
echo "To verify installation, run:"
echo "  ls -la $HOLOSCAN_LIBS_DIR | grep gxf_video"
echo ""
echo "If you encounter issues, ensure that:"
echo "  1. NVIDIA GPU drivers are installed"
echo "  2. CUDA is properly configured"
echo "  3. Holoscan SDK is installed in /opt/nvidia/holoscan/"
echo "  4. You have sufficient permissions to write to system directories" 