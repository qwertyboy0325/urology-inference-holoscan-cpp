#!/bin/bash

# 基於 NGC 官方文檔的 Holoscan 容器運行腳本
# 參考: https://catalog.ngc.nvidia.com/orgs/nvidia/teams/clara-holoscan/containers/holoscan

set -e

echo "🚀 使用 NGC 官方方法運行 Holoscan 容器..."

# 設置容器鏡像路徑
export NGC_CONTAINER_IMAGE_PATH="nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu"

echo "📋 容器鏡像: $NGC_CONTAINER_IMAGE_PATH"
echo "📋 當前 DISPLAY: $DISPLAY"

# 停止現有容器
echo "🛑 停止現有容器..."
docker compose down 2>/dev/null || true
docker stop urology-dev 2>/dev/null || true
docker rm urology-dev 2>/dev/null || true

# 使用官方推薦的 X11 設置方法
echo "🔓 設置 X11 權限..."
xhost +local:docker 2>/dev/null || echo "⚠️  xhost 設置失敗（SSH 環境中正常）"

# 使用官方文檔中的完整 Docker 運行命令
echo "🚀 啟動官方 Holoscan 容器..."
docker run -it --rm --net host \
  --runtime=nvidia \
  --gpus all \
  --ipc=host --cap-add=CAP_SYS_PTRACE --ulimit memlock=-1 --ulimit stack=67108864 \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -e DISPLAY \
  -v $(pwd):/workspace \
  -w /workspace \
  --name urology-dev \
  ${NGC_CONTAINER_IMAGE_PATH} \
  bash -c "
    echo '=== 進入 Holoscan 官方容器 ==='
    echo 'DISPLAY=' \$DISPLAY
    echo 'GPU 信息:'
    nvidia-smi --query-gpu=name --format=csv,noheader,nounits 2>/dev/null || echo '無法獲取 GPU 信息'
    
    echo '=== 測試 Holoscan 安裝 ==='
    echo 'Holoscan 安裝路徑:'
    ls -la /opt/nvidia/holoscan/ | head -5
    
    echo '=== 測試 Hello World ==='
    /opt/nvidia/holoscan/examples/hello_world/cpp/hello_world
    
    echo '=== 構建你的應用程序 ==='
    cd /workspace
    if [ ! -d build ]; then
        mkdir -p build
        cd build
        cmake .. -DCMAKE_BUILD_TYPE=Release
        make -j\$(nproc)
    else
        cd build
    fi
    
    echo '=== 運行 Urology Inference ==='
    echo '可用的執行檔:'
    ls -la *inference* *test* 2>/dev/null || echo '沒有找到執行檔'
    
    echo '嘗試運行簡化推理測試:'
    if [ -f ./simple_inference_test ]; then
        ./simple_inference_test
    else
        echo '簡化測試不存在，嘗試構建...'
        make simple_inference_test -j\$(nproc) 2>/dev/null || echo '構建失敗'
        if [ -f ./simple_inference_test ]; then
            ./simple_inference_test
        fi
    fi
    
    echo '=== 進入互動模式 ==='
    echo '你現在可以運行:'
    echo '1. ./simple_inference_test                    # 簡化推理測試'
    echo '2. HOLOVIZ_HEADLESS=1 ./urology_inference_holoscan_cpp --data=../data  # Headless 模式'
    echo '3. ./urology_inference_holoscan_cpp --data=../data                      # 完整應用'
    echo ''
    bash
  " 