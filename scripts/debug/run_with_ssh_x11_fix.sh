#!/bin/bash

# 🎯 專門針對 SSH X11 轉發的 Holoscan 容器啟動腳本
# 解決 SSH 環境中的 X11 轉發問題

set -e

echo "🚀 SSH X11 轉發專用版本啟動..."

# 1. 檢查 SSH X11 轉發環境
echo "📋 檢查 SSH X11 轉發環境..."
echo "DISPLAY: $DISPLAY"
echo "SSH_CLIENT: $SSH_CLIENT"
echo "SSH_CONNECTION: $SSH_CONNECTION"

# 2. 設置正確的 DISPLAY 變數
if [ -z "$DISPLAY" ]; then
    # 如果 DISPLAY 為空，嘗試自動檢測
    if [ -n "$SSH_CLIENT" ]; then
        export DISPLAY=localhost:10.0
        echo "🔧 自動設置 DISPLAY 為 SSH 轉發: $DISPLAY"
    else
        export DISPLAY=:0
        echo "🔧 設置 DISPLAY 為本地顯示: $DISPLAY"
    fi
else
    echo "✅ 使用現有 DISPLAY: $DISPLAY"
fi

# 3. 強制清理現有容器
echo "🛑 清理現有容器..."
docker stop holoscan-dev-taiwan 2>/dev/null || true
docker rm holoscan-dev-taiwan 2>/dev/null || true

# 4. 跳過 xhost 設置（SSH 環境中不需要）
echo "⚠️  跳過 xhost 設置（SSH 環境中使用認證文件）"

# 5. 查找 nvidia_icd.json
echo "🔍 查找 nvidia_icd.json..."
nvidia_icd_json=$(find /usr/share /etc -path '*/vulkan/icd.d/nvidia_icd.json' -type f,l -print -quit 2>/dev/null | grep .) || nvidia_icd_json="/usr/share/vulkan/icd.d/nvidia_icd.json"
echo "✅ 使用 nvidia_icd.json: $nvidia_icd_json"

# 6. 設置 X11 環境變數
echo "🔑 設置 X11 環境變數..."
export XSOCK=/tmp/.X11-unix
export XAUTH=/tmp/.docker.xauth.ssh

# 7. 為 SSH X11 轉發創建特殊的認證文件
echo "📝 創建 SSH X11 認證文件..."
sudo rm -f $XAUTH

# SSH X11 轉發的特殊處理
if [ -n "$SSH_CLIENT" ] && [ -n "$DISPLAY" ]; then
    echo "🔐 SSH 環境：創建 X11 認證文件..."
    
    # 方法1：嘗試從 SSH 會話獲取認證
    if [ -f "$HOME/.Xauthority" ]; then
        echo "📋 複製用戶 .Xauthority 文件..."
        sudo cp "$HOME/.Xauthority" "$XAUTH"
    else
        echo "📋 .Xauthority 不存在，創建新的認證文件..."
        # 嘗試從當前 DISPLAY 獲取認證
        if xauth list $DISPLAY 2>/dev/null | head -1 | sudo xauth -f $XAUTH nmerge - 2>/dev/null; then
            echo "✅ 成功從當前 DISPLAY 獲取認證"
        else
            echo "⚠️  無法獲取認證，創建空認證文件"
            sudo touch $XAUTH
            # 為 SSH 轉發添加通用認證
            echo "add $DISPLAY . $(mcookie)" | sudo xauth -f $XAUTH source - 2>/dev/null || true
        fi
    fi
else
    echo "🖥️  本地環境：使用標準方法..."
    xauth nlist $DISPLAY 2>/dev/null | sed -e 's/^..../ffff/' | sudo xauth -f $XAUTH nmerge - 2>/dev/null || sudo touch $XAUTH
fi

sudo chmod 777 $XAUTH
echo "✅ 認證文件創建完成: $XAUTH"

# 8. 啟動容器（SSH X11 優化版本）
echo "🚀 啟動容器（SSH X11 優化版本）..."
docker run -it --rm \
  --name holoscan-dev-taiwan \
  --gpus all \
  --net host \
  --ipc host \
  --pid host \
  -v $XAUTH:$XAUTH \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v $nvidia_icd_json:$nvidia_icd_json:ro \
  -e DISPLAY=$DISPLAY \
  -e XAUTHORITY=$XAUTH \
  -e NVIDIA_DRIVER_CAPABILITIES=graphics,video,compute,utility,display \
  -e LIBGL_ALWAYS_INDIRECT=1 \
  -e MESA_GL_VERSION_OVERRIDE=3.3 \
  --cap-add=CAP_SYS_PTRACE \
  --security-opt seccomp=unconfined \
  --ulimit memlock=-1 \
  -v "$(pwd):/workspace" \
  -w /workspace \
  nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu \
  bash -c "
    echo '=== SSH X11 轉發測試 ==='
    echo '📋 容器內 X11 環境：'
    echo 'DISPLAY=' \$DISPLAY
    echo 'XAUTHORITY=' \$XAUTHORITY
    echo 'LIBGL_ALWAYS_INDIRECT=' \$LIBGL_ALWAYS_INDIRECT
    echo ''
    
    # 修復編譯器問題
    echo '🔧 修復編譯器配置...'
    export CC=/usr/bin/gcc
    export CXX=/usr/bin/g++
    export CMAKE_C_COMPILER=/usr/bin/gcc
    export CMAKE_CXX_COMPILER=/usr/bin/g++
    echo ''
    
    # 安裝必要工具
    echo '🧪 安裝 X11 測試工具...'
    apt update -qq && apt install -y x11-apps xauth
    echo ''
    
    # 測試 X11 連接
    echo '🎯 測試 X11 連接...'
    echo '檢查 X11 socket:'
    ls -la /tmp/.X11-unix/ || echo '❌ X11 socket 不存在'
    echo ''
    echo '檢查認證文件:'
    ls -la \$XAUTHORITY || echo '❌ 認證文件不存在'
    echo ''
    echo '測試 xauth:'
    xauth list 2>/dev/null || echo '⚠️  xauth 列表為空'
    echo ''
    echo '測試 X11 應用:'
    timeout 3 xeyes 2>/dev/null &
    XEYES_PID=\$!
    sleep 1
    if kill -0 \$XEYES_PID 2>/dev/null; then
        echo '✅ X11 連接成功！xeyes 正在運行'
        kill \$XEYES_PID 2>/dev/null || true
    else
        echo '❌ xeyes 啟動失敗'
    fi
    echo ''
    
    # 測試 Holoscan Hello World
    echo '🎯 測試 Holoscan Hello World...'
    /opt/nvidia/holoscan/examples/hello_world/cpp/hello_world
    echo ''
    
    # 重新構建項目
    echo '🔨 重新構建項目...'
    cd /workspace
    rm -rf build
    mkdir build
    cd build
    if cmake .. && make -j\$(nproc); then
        echo '✅ 項目構建成功'
        echo ''
        echo '🎯 測試完整應用（UI 模式）:'
        echo './urology_inference_holoscan_cpp --data=../data'
        echo ''
        echo '🎯 測試完整應用（Headless 模式）:'
        echo 'HOLOVIZ_HEADLESS=1 ./urology_inference_holoscan_cpp --data=../data'
        echo ''
        echo '🧪 簡化測試:'
        echo './simple_inference_test'
    else
        echo '❌ 項目構建失敗'
    fi
    echo ''
    
    echo '🎯 容器已準備就緒！進入交互模式...'
    /bin/bash
  " 