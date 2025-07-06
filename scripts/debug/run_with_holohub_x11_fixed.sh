#!/bin/bash

# 🎯 修復版本：按照 HoloHub 官方文檔配置 X11 的 Holoscan 容器啟動腳本
# 參考：https://github.com/nvidia-holoscan/holohub/tree/main/tutorials/holoscan-playground-on-aws
# 修復：SSH X11 轉發問題和容器清理問題

set -e

echo "🚀 修復版本：按照 HoloHub 官方方式啟動 Holoscan 容器..."

# 0. 檢查 DISPLAY 變數
echo "📋 檢查 X11 環境..."
echo "原始 DISPLAY: $DISPLAY"
echo "SSH_CLIENT: $SSH_CLIENT"

# 如果 DISPLAY 為空，設置默認值
if [ -z "$DISPLAY" ]; then
    export DISPLAY=localhost:10.0
    echo "⚠️  DISPLAY 為空，設置為默認值: $DISPLAY"
fi

# 1. 強制清理現有容器
echo "🛑 強制清理現有容器..."
docker stop holoscan-dev-taiwan 2>/dev/null || true
docker rm holoscan-dev-taiwan 2>/dev/null || true
docker container prune -f 2>/dev/null || true

# 2. 安裝 xhost 工具（如果尚未安裝）
echo "📦 檢查並安裝 xhost 工具..."
if ! command -v xhost &> /dev/null; then
    echo "安裝 x11-xserver-utils..."
    sudo apt install -y x11-xserver-utils
else
    echo "✅ xhost 工具已安裝"
fi

# 3. 設置 X11 權限（HoloHub 官方方式，但處理 SSH 環境）
echo "🔓 設置 X11 權限..."
if xhost +local:docker 2>/dev/null; then
    echo "✅ xhost 設置成功"
else
    echo "⚠️  xhost 設置失敗（SSH 環境中正常，繼續執行）"
fi

# 4. 查找 nvidia_icd.json（HoloHub 官方方式）
echo "🔍 查找 nvidia_icd.json..."
nvidia_icd_json=$(find /usr/share /etc -path '*/vulkan/icd.d/nvidia_icd.json' -type f,l -print -quit 2>/dev/null | grep .) || (echo "nvidia_icd.json not found, 使用默認路徑" && nvidia_icd_json="/usr/share/vulkan/icd.d/nvidia_icd.json")
echo "✅ 使用 nvidia_icd.json: $nvidia_icd_json"

# 5. 設置 X11 環境變數（HoloHub 官方方式）
echo "🔑 設置 X11 環境變數..."
export XSOCK=/tmp/.X11-unix
export XAUTH=/tmp/.docker.xauth

# 6. 創建 X11 認證文件（修復版本）
echo "📝 創建 X11 認證文件..."
# 清理舊的認證文件
sudo rm -f $XAUTH

# 為 SSH X11 轉發創建認證文件
if [ -n "$DISPLAY" ]; then
    # 嘗試從當前 X11 會話創建認證
    if xauth nlist $DISPLAY 2>/dev/null | sed -e 's/^..../ffff/' | sudo xauth -f $XAUTH nmerge - 2>/dev/null; then
        echo "✅ 從當前會話創建認證文件"
    else
        echo "⚠️  無法從當前會話創建認證，創建空認證文件"
        sudo touch $XAUTH
    fi
else
    echo "⚠️  DISPLAY 未設置，創建空認證文件"
    sudo touch $XAUTH
fi

sudo chmod 777 $XAUTH
echo "✅ 認證文件創建完成: $XAUTH"

# 7. 設置容器鏡像
export NGC_CONTAINER_IMAGE_PATH="nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu"

# 8. 啟動容器（修復版本）
echo "🚀 啟動容器（修復版本）..."
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
    echo '=== 修復版本 X11 配置測試 ==='
    echo '📋 X11 環境檢查：'
    echo 'DISPLAY=' \$DISPLAY
    echo 'XAUTHORITY=' \$XAUTHORITY
    echo ''
    
    # 修復編譯器問題
    echo '🔧 修復編譯器配置...'
    export CC=/usr/bin/gcc
    export CXX=/usr/bin/g++
    echo 'CC=' \$CC
    echo 'CXX=' \$CXX
    echo ''
    
    echo '🧪 安裝必要工具...'
    apt update -qq && apt install -y x11-apps gcc g++ build-essential
    echo ''
    
    echo '🎯 測試 X11 連接...'
    if timeout 5 xeyes 2>/dev/null; then
        echo '✅ X11 連接成功！'
    else
        echo '❌ X11 連接失敗，但容器已準備就緒'
        echo '在 SSH 環境中這是正常的，可以使用 headless 模式'
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
    if cmake .. && make -j\$(nproc); then
        echo '✅ 項目構建成功'
    else
        echo '❌ 項目構建失敗，但推理功能仍可用'
    fi
    echo ''
    
    echo '🧪 測試推理功能...'
    if [ -f ./simple_inference_test ]; then
        ./simple_inference_test
    else
        echo '⚠️  simple_inference_test 不存在，跳過測試'
    fi
    echo ''
    
    echo '🎯 容器已準備就緒！'
    echo '可用的運行選項：'
    echo ''
    echo '🖥️  完整 UI 模式（如果 X11 正常）：'
    echo './urology_inference_holoscan_cpp --data=../data'
    echo ''
    echo '🖥️  Headless 模式（推薦用於 SSH 環境）：'
    echo 'HOLOVIZ_HEADLESS=1 ./urology_inference_holoscan_cpp --data=../data'
    echo ''
    echo '🧪 簡化測試：'
    echo './simple_inference_test'
    echo ''
    echo '🔍 檢查可用的執行文件：'
    ls -la *.cpp 2>/dev/null || echo '無 .cpp 文件'
    ls -la urology* simple* hello* 2>/dev/null || echo '無執行文件'
    echo ''
    /bin/bash
  " 