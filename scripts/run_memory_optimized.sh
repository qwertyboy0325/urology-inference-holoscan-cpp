#!/bin/bash

# 🚀 Run Urology Inference with Maximum Memory Optimization
# This script runs the application with aggressive memory optimization

set -e

echo "🚀 Starting Urology Inference application (maximum memory optimization mode)..."

# Check if data directory exists
if [ ! -d "data" ]; then
    echo "❌ Error: data directory does not exist"
    exit 1
fi

# Check for model file
MODEL_FILE="data/models/urology_yolov9c_3000random640resize_20240811_4.34_nhwc.onnx"
if [ ! -f "$MODEL_FILE" ]; then
    echo "❌ Error: model file does not exist: $MODEL_FILE"
    exit 1
fi

echo "✅ Model file found: $MODEL_FILE"

# Check for input data
if [ ! -d "data/inputs" ] || [ -z "$(ls -A data/inputs 2>/dev/null)" ]; then
    echo "❌ Error: data/inputs directory is empty or does not exist"
    exit 1
fi

# Show input file sizes
echo "📁 Input file check:"
ls -lh data/inputs/
echo ""

# Create output directory
mkdir -p data/output
mkdir -p data/logs

echo "🛑 Cleaning up existing container..."
docker stop urology-dev-x11 2>/dev/null || true
docker rm urology-dev-x11 2>/dev/null || true

echo "🔨 Building development image..."
docker compose build urology-dev

echo "🚀 Starting container (maximum memory optimization)..."
docker run -it --rm \
  --name urology-dev-x11 \
  --gpus all \
  --net host \
  --ipc host \
  --pid host \
  --memory=16g \
  --memory-swap=32g \
  --shm-size=16g \
  --memory-reservation=12g \
  -v /tmp/.docker.xauth.urology:/tmp/.docker.xauth.urology \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v /usr/share/vulkan/icd.d/nvidia_icd.json:/usr/share/vulkan/icd.d/nvidia_icd.json:ro \
  -v "$(pwd):/workspace" \
  -v "./data:/workspace/data" \
  -w /workspace \
  -e DISPLAY=:10.0 \
  -e XAUTHORITY=/tmp/.docker.xauth.urology \
  -e NVIDIA_DRIVER_CAPABILITIES=graphics,video,compute,utility,display \
  -e LIBGL_ALWAYS_INDIRECT=1 \
  -e MESA_GL_VERSION_OVERRIDE=3.3 \
  -e CMAKE_C_COMPILER=/usr/bin/gcc \
  -e CMAKE_CXX_COMPILER=/usr/bin/g++ \
  -e CC=/usr/bin/gcc \
  -e CXX=/usr/bin/g++ \
  -e HOLOVIZ_HEADLESS=1 \
  -e CUDA_VISIBLE_DEVICES=0 \
  --cap-add=CAP_SYS_PTRACE \
  --security-opt seccomp=unconfined \
  --ulimit memlock=-1 \
  --ulimit stack=67108864 \
  urology-inference-holoscan-cpp-urology-dev:latest \
  bash -c "
    echo '=== Urology Inference Maximum Memory Optimization Mode ==='
    echo '📋 Memory configuration:'
    echo 'Container memory limit: 16GB'
    echo 'Swap: 32GB'
    echo 'Shared memory: 16GB'
    echo 'Memory reservation: 12GB'
    echo ''
    
    # Set memory optimization environment variables
    export CUDA_MEMORY_POOL_SIZE=1073741824  # 1GB CUDA memory pool
    export GXF_MEMORY_POOL_SIZE=2147483648   # 2GB GXF memory pool
    export HOLOSCAN_MEMORY_POOL_SIZE=4294967296  # 4GB Holoscan memory pool
    
    # Build project (if needed)
    if [ ! -f build/urology_inference_holoscan_cpp ]; then
        echo '🔨 Building project...'
        mkdir -p build
        cd build
        cmake .. && make -j\$(nproc)
        cd ..
    fi
    
    echo '🎯 Running memory optimization test...'
    cd build
    
    # First, test simple inference
    echo '🧪 Testing simple inference (memory check)...'
    ./simple_inference_test
    echo ''
    
    # Then, run the full application
    echo '🎯 Running full application (memory optimization mode)...'
    ./urology_inference_holoscan_cpp --data=../data
  " 