#!/bin/bash

# 🚀 Run Urology Inference with Increased Memory
# This script runs the application with proper memory allocation for real data processing

set -e

echo "🚀 啟動 Urology Inference 應用程序（高記憶體模式）..."

# Check if data directory exists
if [ ! -d "data" ]; then
    echo "❌ 錯誤：data 目錄不存在"
    echo "請確保 data 目錄包含以下結構："
    echo "  data/"
    echo "  ├── models/     (包含 .onnx 模型文件)"
    echo "  ├── inputs/     (包含 GXF 輸入文件)"
    echo "  └── output/     (輸出目錄)"
    exit 1
fi

# Check for model file
MODEL_FILE="data/models/urology_yolov9c_3000random640resize_20240811_4.34_nhwc.onnx"
if [ ! -f "$MODEL_FILE" ]; then
    echo "❌ 錯誤：模型文件不存在: $MODEL_FILE"
    exit 1
fi

echo "✅ 模型文件找到: $MODEL_FILE"

# Check for input data
if [ ! -d "data/inputs" ] || [ -z "$(ls -A data/inputs 2>/dev/null)" ]; then
    echo "⚠️  警告：data/inputs 目錄為空或不存在"
    echo "應用程序將運行但沒有輸入數據"
fi

# Create output directory
mkdir -p data/output

# Set environment variables
export DISPLAY=${DISPLAY:-:0}
export XAUTH=/tmp/.docker.xauth.urology
export XSOCK=/tmp/.X11-unix

# Create X11 auth file if needed
if [ ! -f "$XAUTH" ]; then
    echo "📝 創建 X11 認證文件..."
    sudo touch $XAUTH
    sudo chmod 777 $XAUTH
    xauth nlist $DISPLAY 2>/dev/null | sed -e 's/^..../ffff/' | sudo xauth -f $XAUTH nmerge - 2>/dev/null || true
fi

# Find nvidia_icd.json
nvidia_icd_json=$(find /usr/share /etc -path '*/vulkan/icd.d/nvidia_icd.json' -type f,l -print -quit 2>/dev/null | grep .) || nvidia_icd_json="/usr/share/vulkan/icd.d/nvidia_icd.json"

# Stop existing container
echo "🛑 清理現有容器..."
docker stop urology-dev-x11 2>/dev/null || true
docker rm urology-dev-x11 2>/dev/null || true

# Build image if needed
echo "🔨 構建開發鏡像..."
docker compose build urology-dev

# Run with increased memory
echo "🚀 啟動高記憶體容器..."
docker run -it --rm \
  --name urology-dev-x11 \
  --gpus all \
  --net host \
  --ipc host \
  --pid host \
  --memory=12g \
  --memory-swap=24g \
  --shm-size=4g \
  -v $XAUTH:$XAUTH \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v $nvidia_icd_json:$nvidia_icd_json:ro \
  -v "$(pwd):/workspace" \
  -v "./data:/workspace/data" \
  -w /workspace \
  -e DISPLAY=$DISPLAY \
  -e XAUTHORITY=$XAUTH \
  -e NVIDIA_DRIVER_CAPABILITIES=graphics,video,compute,utility,display \
  -e LIBGL_ALWAYS_INDIRECT=1 \
  -e MESA_GL_VERSION_OVERRIDE=3.3 \
  -e CMAKE_C_COMPILER=/usr/bin/gcc \
  -e CMAKE_CXX_COMPILER=/usr/bin/g++ \
  -e CC=/usr/bin/gcc \
  -e CXX=/usr/bin/g++ \
  --cap-add=CAP_SYS_PTRACE \
  --security-opt seccomp=unconfined \
  --ulimit memlock=-1 \
  urology-inference-holoscan-cpp-urology-dev:latest \
  bash -c "
    echo '=== Urology Inference 高記憶體模式 ==='
    echo '📋 系統信息：'
    echo '記憶體總量：' \$(free -h | grep Mem | awk '{print \$2}')
    echo '可用記憶體：' \$(free -h | grep Mem | awk '{print \$7}')
    echo 'GPU 記憶體：' \$(nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits | head -1) MB
    echo ''
    
    # Build project
    echo '🔨 構建項目...'
    cd build
    if [ ! -f urology_inference_holoscan_cpp ]; then
        echo '重新構建項目...'
        cmake .. && make -j\$(nproc)
    fi
    echo '✅ 構建完成'
    echo ''
    
    # List input files
    echo '📁 輸入文件檢查：'
    ls -la ../data/inputs/ 2>/dev/null || echo '無輸入文件'
    echo ''
    
    # Run application
    echo '🎯 啟動應用程序...'
    echo '選擇運行模式：'
    echo '1) UI 模式 (X11 顯示)'
    echo '2) Headless 模式 (無 UI)'
    echo '3) 簡單測試模式'
    read -p '請選擇 [1/2/3]: ' mode
    
    case \$mode in
        1)
            echo '🎯 啟動 UI 模式...'
            ./urology_inference_holoscan_cpp --data=../data
            ;;
        2)
            echo '🎯 啟動 Headless 模式...'
            HOLOVIZ_HEADLESS=1 ./urology_inference_holoscan_cpp --data=../data
            ;;
        3)
            echo '🎯 啟動簡單測試模式...'
            ./simple_inference_test
            ;;
        *)
            echo '🎯 啟動 UI 模式...'
            ./urology_inference_holoscan_cpp --data=../data
            ;;
    esac
  " 