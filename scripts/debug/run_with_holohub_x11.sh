#!/bin/bash

# 🎯 按照 HoloHub 官方文檔配置 X11 的 Holoscan 容器啟動腳本
# 參考：https://github.com/nvidia-holoscan/holohub/tree/main/tutorials/holoscan-playground-on-aws

set -e

echo "🚀 按照 HoloHub 官方方式啟動 Holoscan 容器..."

# 1. 安裝 xhost 工具（如果尚未安裝）
echo "📦 檢查並安裝 xhost 工具..."
if ! command -v xhost &> /dev/null; then
    echo "安裝 x11-xserver-utils..."
    sudo apt install -y x11-xserver-utils
else
    echo "✅ xhost 工具已安裝"
fi

# 2. 設置 X11 權限（HoloHub 官方方式）
echo "🔓 設置 X11 權限..."
xhost +local:docker || echo "⚠️  xhost 設置失敗（SSH 環境中可能正常）"

# 3. 查找 nvidia_icd.json（HoloHub 官方方式）
echo "🔍 查找 nvidia_icd.json..."
nvidia_icd_json=$(find /usr/share /etc -path '*/vulkan/icd.d/nvidia_icd.json' -type f,l -print -quit 2>/dev/null | grep .) || (echo "nvidia_icd.json not found" >&2 && false)
echo "✅ 找到 nvidia_icd.json: $nvidia_icd_json"

# 4. 設置 X11 環境變數（HoloHub 官方方式）
echo "🔑 設置 X11 環境變數..."
export XSOCK=/tmp/.X11-unix
export XAUTH=/tmp/.docker.xauth

# 5. 創建 X11 認證文件（HoloHub 官方方式）
echo "📝 創建 X11 認證文件..."
# 這個錯誤是預期的，根據 HoloHub 文檔
xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | sudo xauth -f $XAUTH nmerge - || echo "⚠️  認證合併失敗（預期行為）"
sudo chmod 777 $XAUTH
echo "✅ 認證文件創建完成: $XAUTH"

# 6. 清理現有容器
echo "🛑 清理現有容器..."
docker stop holoscan-dev-taiwan 2>/dev/null || true
docker rm holoscan-dev-taiwan 2>/dev/null || true

# 7. 設置容器鏡像
export NGC_CONTAINER_IMAGE_PATH="nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu"

# 8. 啟動容器（完全按照 HoloHub 官方方式）
echo "🚀 啟動容器（HoloHub 官方配置）..."
docker run -it --rm --net host \
  --name holoscan-dev-taiwan \
  --gpus all \
  -v $XAUTH:$XAUTH -e XAUTHORITY=$XAUTH \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v $nvidia_icd_json:$nvidia_icd_json:ro \
  -e NVIDIA_DRIVER_CAPABILITIES=graphics,video,compute,utility,display \
  -e DISPLAY=$DISPLAY \
  --ipc=host \
  --cap-add=CAP_SYS_PTRACE \
  --ulimit memlock=-1 \
  -v "$(pwd):/workspace" \
  -w /workspace \
  ${NGC_CONTAINER_IMAGE_PATH} \
  bash -c "
    echo '=== HoloHub 官方 X11 配置測試 ==='
    echo '📋 X11 環境檢查：'
    echo 'DISPLAY=' \$DISPLAY
    echo 'XAUTHORITY=' \$XAUTHORITY
    echo ''
    echo '🧪 安裝 X11 測試工具...'
    apt update -qq && apt install -y x11-apps
    echo ''
    echo '🎯 測試 X11 連接...'
    if timeout 5 xeyes 2>/dev/null; then
        echo '✅ X11 連接成功！'
    else
        echo '❌ X11 連接失敗，但容器已準備就緒'
    fi
    echo ''
    echo '🎯 測試 Holoscan Hello World...'
    /opt/nvidia/holoscan/examples/hello_world/cpp/hello_world
    echo ''
    echo '🔨 構建項目...'
    cd /workspace
    if [ ! -d build ]; then
        mkdir build
    fi
    cd build
    cmake .. && make -j\$(nproc)
    echo ''
    echo '🧪 測試推理功能...'
    ./simple_inference_test
    echo ''
    echo '🎯 容器已準備就緒！'
    echo '可用的運行選項：'
    echo ''
    echo '🖥️  完整 UI 模式（如果 X11 正常）：'
    echo './urology_inference_holoscan_cpp --data=../data'
    echo ''
    echo '🖥️  Headless 模式（推薦）：'
    echo 'HOLOVIZ_HEADLESS=1 ./urology_inference_holoscan_cpp --data=../data'
    echo ''
    echo '🧪 簡化測試：'
    echo './simple_inference_test'
    echo ''
    /bin/bash
  " 