# Urology Inference Holoscan C++ - Installation Guide

This guide provides step-by-step instructions for installing and setting up the Urology Inference Holoscan C++ application.

## Prerequisites

### Hardware Requirements
- NVIDIA GPU with Compute Capability 6.0+ (Pascal architecture or newer)
- Minimum 8GB GPU memory (recommended 12GB+)
- 16GB+ system RAM
- SSD storage (recommended for better I/O performance)

### Software Requirements
- Ubuntu 20.04 LTS or Ubuntu 22.04 LTS
- NVIDIA Driver 525+ (for CUDA 12.x support)
- Docker (if using containerized deployment)

## Step 1: Install NVIDIA Holoscan SDK

### Option A: Using Package Manager (Recommended)
```bash
# Add NVIDIA package repository
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/cuda-keyring_1.0-1_all.deb
sudo dpkg -i cuda-keyring_1.0-1_all.deb
sudo apt update

# Install Holoscan SDK
sudo apt install holoscan
```

### Option B: From Source
```bash
# Clone Holoscan repository
git clone https://github.com/nvidia-holoscan/holoscan-sdk.git
cd holoscan-sdk

# Build and install
./scripts/build.sh --type Release
sudo ./scripts/install.sh
```

## Step 2: Install System Dependencies

```bash
# Update package repository
sudo apt update

# Install build tools
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config

# Install required libraries
sudo apt install -y \
    libopencv-dev \
    libyaml-cpp-dev \
    libcuda1 \
    libnvidia-decode-470 \
    libnvidia-encode-470

# Install optional dependencies (for Qt GUI)
sudo apt install -y \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-multimedia-dev
```

## Step 3: Verify CUDA Installation

```bash
# Check NVIDIA driver
nvidia-smi

# Check CUDA installation
nvcc --version

# Test CUDA samples (optional)
cd /usr/local/cuda/samples/1_Utilities/deviceQuery
sudo make
./deviceQuery
```

## Step 4: Clone and Build the Application

```bash
# Clone the repository
git clone <repository-url> urology-inference-holoscan-cpp
cd urology-inference-holoscan-cpp

# Create data directories
mkdir -p data/{models,inputs,output}

# Build the application
./scripts/build.sh

# Verify build
ls build/urology_inference_holoscan_cpp
```

## Step 5: Model Setup

### Download Model File
```bash
# Contact your system administrator for the model file
# Place the model in the data/models directory:
# data/models/Urology_yolov11x-seg_3-13-16-17val640rezize_1_4.40.0_nms.onnx
```

### Verify Model
```bash
# Check model file exists and has correct size
ls -lh data/models/Urology_yolov11x-seg_3-13-16-17val640rezize_1_4.40.0_nms.onnx
```

## Step 6: Test Installation

### Basic Test
```bash
# Run with help to verify installation
./scripts/run.sh --help
```

### Sample Data Test (if available)
```bash
# Copy sample video to inputs directory
cp /path/to/sample/video.mp4 data/inputs/

# Run the application
./scripts/run.sh -d ./data --log-level INFO
```

## Troubleshooting

### Common Issues

#### 1. Holoscan SDK Not Found
```bash
# Check PKG_CONFIG_PATH
echo $PKG_CONFIG_PATH

# Add Holoscan to path if needed
export PKG_CONFIG_PATH="/opt/nvidia/holoscan/lib/pkgconfig:$PKG_CONFIG_PATH"
```

#### 2. CUDA/GPU Issues
```bash
# Check GPU visibility
nvidia-smi

# Check CUDA runtime
ldconfig -p | grep cuda

# Test GPU memory
nvidia-ml-py3 --query-gpu=name,memory.total,memory.free --format=csv
```

#### 3. Build Errors
```bash
# Clean build
./scripts/build.sh clean

# Build with debug info
./scripts/build.sh debug

# Check CMake log
cat build/CMakeCache.txt | grep ERROR
```

#### 4. Runtime Errors
```bash
# Check library dependencies
ldd build/urology_inference_holoscan_cpp

# Run with debug logging
HOLOSCAN_LOG_LEVEL=DEBUG ./scripts/run.sh
```

### Performance Optimization

#### GPU Memory Optimization
```bash
# Monitor GPU memory usage
watch -n 1 nvidia-smi

# Adjust memory pool sizes in config if needed
```

#### CPU Performance
```bash
# Check CPU utilization
htop

# Set CPU affinity if needed
taskset -c 0-7 ./scripts/run.sh
```

## Environment Configuration

### Environment Variables
Add to your `~/.bashrc` or `~/.profile`:

```bash
# Holoscan SDK
export HOLOSCAN_INSTALL_PATH="/opt/nvidia/holoscan"
export PKG_CONFIG_PATH="$HOLOSCAN_INSTALL_PATH/lib/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="$HOLOSCAN_INSTALL_PATH/lib:$LD_LIBRARY_PATH"

# Application specific
export HOLOHUB_DATA_PATH="/path/to/your/data"
export HOLOSCAN_LOG_LEVEL="INFO"
```

### System Configuration

#### GPU Performance Mode
```bash
# Set GPU to performance mode
sudo nvidia-smi -pm 1
sudo nvidia-smi -ac 877,1215  # Adjust for your GPU
```

#### System Limits
Add to `/etc/security/limits.conf`:
```
* soft memlock unlimited
* hard memlock unlimited
```

## Production Deployment

### Systemd Service (Optional)
Create `/etc/systemd/system/urology-inference.service`:

```ini
[Unit]
Description=Urology Inference Holoscan Application
Requires=nvidia-persistenced.service
After=nvidia-persistenced.service

[Service]
Type=simple
User=urology
Group=urology
WorkingDirectory=/opt/urology-inference-holoscan-cpp
Environment=HOLOHUB_DATA_PATH=/opt/urology-inference-holoscan-cpp/data
Environment=HOLOSCAN_LOG_LEVEL=INFO
ExecStart=/opt/urology-inference-holoscan-cpp/scripts/run.sh
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl enable urology-inference.service
sudo systemctl start urology-inference.service
```

## Support

For additional support:
1. Check the troubleshooting section above
2. Review application logs in `/tmp/holoscan_*.log`
3. Consult the Holoscan SDK documentation
4. Contact the development team

## Next Steps

After successful installation:
1. Review the main [README.md](README.md) for usage instructions
2. Check the [data/README.md](data/README.md) for data setup
3. Configure the application via `src/config/app_config.yaml`
4. Test with your specific medical imaging data 