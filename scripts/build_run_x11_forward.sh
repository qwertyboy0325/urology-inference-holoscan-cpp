#!/bin/bash

# 🎯 整合成功 X11 解決方案到原本的 Docker 環境
# 使用預裝所有依賴的 development stage

set -e

echo "🚀 啟動具有完整 X11 支持的 Urology Inference 開發環境..."

# 1. 檢查 SSH X11 轉發環境
echo "📋 檢查 SSH X11 轉發環境..."
echo "DISPLAY: $DISPLAY"
echo "SSH_CLIENT: $SSH_CLIENT"
echo "SSH_CONNECTION: $SSH_CONNECTION"

# 2. 設置正確的 DISPLAY 變數
if [ -z "$DISPLAY" ]; then
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

# 3. 設置 X11 環境變數
echo "🔑 設置 X11 環境變數..."
export XSOCK=/tmp/.X11-unix
export XAUTH=/tmp/.docker.xauth.urology

# 4. 創建 SSH X11 認證文件（使用成功的方法）
echo "📝 創建 SSH X11 認證文件..."
sudo rm -f $XAUTH

if [ -n "$SSH_CLIENT" ] && [ -n "$DISPLAY" ]; then
    echo "🔐 SSH 環境：創建 X11 認證文件..."
    if [ -f "$HOME/.Xauthority" ]; then
        echo "📋 複製用戶 .Xauthority 文件..."
        sudo cp "$HOME/.Xauthority" "$XAUTH"
    else
        echo "📋 .Xauthority 不存在，創建新的認證文件..."
        if xauth list $DISPLAY 2>/dev/null | head -1 | sudo xauth -f $XAUTH nmerge - 2>/dev/null; then
            echo "✅ 成功從當前 DISPLAY 獲取認證"
        else
            echo "⚠️  無法獲取認證，創建空認證文件"
            sudo touch $XAUTH
            echo "add $DISPLAY . $(mcookie)" | sudo xauth -f $XAUTH source - 2>/dev/null || true
        fi
    fi
else
    echo "🖥️  本地環境：使用標準方法..."
    xauth nlist $DISPLAY 2>/dev/null | sed -e 's/^..../ffff/' | sudo xauth -f $XAUTH nmerge - 2>/dev/null || sudo touch $XAUTH
fi

sudo chmod 777 $XAUTH
echo "✅ 認證文件創建完成: $XAUTH"

# 5. 查找 nvidia_icd.json
echo "🔍 查找 nvidia_icd.json..."
nvidia_icd_json=$(find /usr/share /etc -path '*/vulkan/icd.d/nvidia_icd.json' -type f,l -print -quit 2>/dev/null | grep .) || nvidia_icd_json="/usr/share/vulkan/icd.d/nvidia_icd.json"
echo "✅ 使用 nvidia_icd.json: $nvidia_icd_json"

# 6. 清理現有容器
echo "🛑 清理現有容器..."
docker compose down 2>/dev/null || true
docker stop urology-dev 2>/dev/null || true
docker rm urology-dev 2>/dev/null || true

# 7. 構建開發鏡像（如果需要）
echo "🔨 構建開發鏡像..."
docker compose build urology-dev

# 8. 啟動具有完整 X11 支持的開發容器
echo "🚀 啟動開發容器（完整 X11 支持）..."
docker run -it --rm \
  --name urology-dev-x11 \
  --gpus all \
  --net host \
  --ipc host \
  --pid host \
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
    echo '=== Urology Inference 開發環境（完整 X11 支持）==='
    echo '📋 環境檢查：'
    echo 'DISPLAY=' \$DISPLAY
    echo 'XAUTHORITY=' \$XAUTHORITY
    echo 'OpenCV 版本：' \$(pkg-config --modversion opencv4)
    echo 'Holoscan 版本：3.3.0'
    echo ''
    
    # 安裝 X11 測試工具
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
    
    # 構建項目
    echo '🔨 構建項目...'
    if [ ! -d build ]; then
        mkdir build
    fi
    cd build
    if cmake .. && make -j\$(nproc); then
        echo '✅ 項目構建成功！'
        echo ''
        echo '🎯 可用的運行選項：'
        echo ''
        echo '🖥️  完整 UI 模式（X11 顯示）：'
        echo './urology_inference_holoscan_cpp --data=../data'
        echo ''
        echo '🖥️  Headless 模式（無 UI）：'
        echo 'HOLOVIZ_HEADLESS=1 ./urology_inference_holoscan_cpp --data=../data'
        echo ''
        echo '🧪 簡化測試：'
        echo './simple_inference_test'
        echo ''
        echo '📁 可用的執行文件：'
        ls -la *urology* *simple* *hello* 2>/dev/null || echo '無執行文件'
        echo ''
        echo '🎯 立即測試完整應用（UI 模式）？'
        echo '輸入 y 測試 UI 模式，輸入 h 測試 headless 模式，或按 Enter 進入交互模式'
        read -t 10 -p '選擇 [y/h/Enter]: ' choice || choice=\"\"
        case \$choice in
            y|Y)
                echo '🎯 測試完整應用（UI 模式）...'
                ./urology_inference_holoscan_cpp --data=../data
                ;;
            h|H)
                echo '🎯 測試完整應用（Headless 模式）...'
                HOLOVIZ_HEADLESS=1 ./urology_inference_holoscan_cpp --data=../data
                ;;
            *)
                echo '🎯 進入交互模式...'
                ;;
        esac
    else
        echo '❌ 項目構建失敗'
    fi
    echo ''
    echo '🎯 開發環境已準備就緒！'
    /bin/bash
  " 