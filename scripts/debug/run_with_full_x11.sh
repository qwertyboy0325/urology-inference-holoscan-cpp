#!/bin/bash

# 完整 X11 支持的 Holoscan 容器啟動腳本
# 在啟動時就配置好所有 X11 需求

set -e

echo "🚀 啟動完整 X11 支持的 Holoscan 容器..."

# 1. 設置環境變量
export NGC_CONTAINER_IMAGE_PATH="nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu"
export DISPLAY=${DISPLAY:-localhost:10.0}

echo "📋 環境配置："
echo "容器鏡像: $NGC_CONTAINER_IMAGE_PATH"
echo "DISPLAY: $DISPLAY"
echo "SSH_CLIENT: $SSH_CLIENT"

# 2. 停止現有容器
echo "🛑 清理現有容器..."
docker compose down 2>/dev/null || true
docker stop urology-dev 2>/dev/null || true
docker rm urology-dev 2>/dev/null || true

# 3. 創建 X11 認證文件（在主機上）
echo "🔑 在主機上創建 X11 認證文件..."
export XAUTH_FILE="/tmp/.docker.xauth.$$"
touch $XAUTH_FILE
chmod 666 $XAUTH_FILE

# 嘗試從當前顯示獲取認證
if [ -n "$DISPLAY" ]; then
    echo "嘗試獲取 X11 認證..."
    xauth nlist $DISPLAY 2>/dev/null | sed -e 's/^..../ffff/' | xauth -f $XAUTH_FILE nmerge - 2>/dev/null || {
        echo "⚠️  無法獲取認證，創建假認證..."
        echo "localhost:10.0 MIT-MAGIC-COOKIE-1 $(openssl rand -hex 16)" | xauth -f $XAUTH_FILE nmerge - 2>/dev/null || true
    }
fi

echo "✅ 認證文件創建：$XAUTH_FILE"
ls -la $XAUTH_FILE

# 4. 設置 X11 權限（如果可能）
echo "🔓 設置 X11 權限..."
xhost +local:docker 2>/dev/null || echo "⚠️  xhost 設置失敗（SSH 環境中正常）"

# 5. 啟動容器並在啟動時配置 X11
echo "🚀 啟動容器並配置 X11..."
docker run -it --rm \
  --name urology-dev \
  --net host \
  --runtime=nvidia \
  --gpus all \
  --ipc=host \
  --cap-add=CAP_SYS_PTRACE \
  --ulimit memlock=-1 \
  --ulimit stack=67108864 \
  -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
  -v $XAUTH_FILE:/tmp/.docker.xauth:rw \
  -v $(pwd):/workspace \
  -w /workspace \
  -e DISPLAY=$DISPLAY \
  -e XAUTHORITY=/tmp/.docker.xauth \
  -e XDG_RUNTIME_DIR=/tmp/runtime-root \
  $NGC_CONTAINER_IMAGE_PATH \
  bash -c "
    echo '=== 容器啟動並配置 X11 ==='
    
    # 設置運行時目錄
    mkdir -p /tmp/runtime-root
    chmod 700 /tmp/runtime-root
    export XDG_RUNTIME_DIR=/tmp/runtime-root
    
    # 檢查環境
    echo '📋 X11 環境檢查：'
    echo 'DISPLAY=' \$DISPLAY
    echo 'XAUTHORITY=' \$XAUTHORITY
    echo 'XDG_RUNTIME_DIR=' \$XDG_RUNTIME_DIR
    
    # 檢查文件
    echo '📁 檢查 X11 文件：'
    ls -la /tmp/.X11-unix/ 2>/dev/null || echo 'X11 socket 不存在'
    ls -la \$XAUTHORITY 2>/dev/null || echo 'XAUTHORITY 文件不存在'
    
    # 安裝 X11 工具
    echo '📦 安裝 X11 工具...'
    apt update -qq && apt install -y x11-apps xauth >/dev/null 2>&1
    
    # 修復認證文件權限
    if [ -f \"\$XAUTHORITY\" ]; then
        chmod 644 \$XAUTHORITY
        echo '✅ 認證文件權限已修復'
    fi
    
    # 測試 X11 連接
    echo '🧪 測試 X11 連接...'
    if timeout 5 xeyes >/dev/null 2>&1; then
        echo '✅ X11 連接成功！'
        X11_WORKING=true
    else
        echo '❌ X11 連接失敗，但容器已準備就緒'
        X11_WORKING=false
        
        # 嘗試修復
        echo '🔄 嘗試修復 X11...'
        unset XAUTHORITY
        if timeout 3 xeyes >/dev/null 2>&1; then
            echo '✅ 無認證文件 X11 連接成功！'
            X11_WORKING=true
        fi
    fi
    
    # 測試 Holoscan
    echo '🎯 測試 Holoscan Hello World...'
    /opt/nvidia/holoscan/examples/hello_world/cpp/hello_world
    
    # 構建項目
    echo '🔨 構建項目...'
    cd /workspace
    if [ ! -d build ]; then
        mkdir -p build
        cd build
        cmake .. -DCMAKE_BUILD_TYPE=Release
        make -j\$(nproc)
    else
        cd build
    fi
    
    # 測試推理
    echo '🧪 測試推理功能...'
    if [ -f ./simple_inference_test ]; then
        ./simple_inference_test
    fi
    
    # 提供運行選項
    echo ''
    echo '🎯 容器已準備就緒！'
    echo '可用的運行選項：'
    echo ''
    if [ \"\$X11_WORKING\" = \"true\" ]; then
        echo '✅ X11 正常工作，可以運行完整應用：'
        echo './urology_inference_holoscan_cpp --data=../data'
        echo ''
    fi
    echo '🖥️  Headless 模式（推薦）：'
    echo 'HOLOVIZ_HEADLESS=1 ./urology_inference_holoscan_cpp --data=../data'
    echo ''
    echo '🧪 簡化測試：'
    echo './simple_inference_test'
    echo ''
    
    # 進入互動模式
    bash
  "

# 清理認證文件
echo "🧹 清理臨時文件..."
rm -f $XAUTH_FILE 2>/dev/null || true 