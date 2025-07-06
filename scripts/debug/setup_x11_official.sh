#!/bin/bash

# 基於 HoloHub 官方文檔的 X11 轉發設置
# 參考: https://github.com/nvidia-holoscan/holohub/tree/main/tutorials/holoscan-playground-on-aws

set -e

echo "🖥️  根據 HoloHub 官方文檔設置 X11 轉發..."

# 1. 檢查是否在 SSH 環境中
if [ -z "$SSH_CLIENT" ] && [ -z "$SSH_CONNECTION" ]; then
    echo "⚠️  未檢測到 SSH 連接，但仍繼續設置 X11..."
fi

# 2. 安裝必要的 X11 工具
echo "🔧 安裝 X11 工具..."
sudo apt update -qq
sudo apt install -y x11-apps x11-xserver-utils xauth

# 3. 設置 DISPLAY 環境變量（如果未設置）
if [ -z "$DISPLAY" ]; then
    echo "⚠️  DISPLAY 未設置，嘗試自動檢測..."
    # 嘗試常見的 DISPLAY 值
    for display in ":0" ":1" "localhost:10.0" "localhost:11.0"; do
        export DISPLAY=$display
        echo "🧪 測試 DISPLAY=$display"
        if timeout 2 xdpyinfo >/dev/null 2>&1; then
            echo "✅ 找到可用的 DISPLAY: $display"
            break
        fi
    done
fi

echo "✅ 當前 DISPLAY: $DISPLAY"

# 4. 設置 X11 socket 和認證文件路徑（根據官方文檔）
export XSOCK=/tmp/.X11-unix
export XAUTH=/tmp/.docker.xauth

echo "🔑 設置 X11 認證文件..."
echo "XSOCK=$XSOCK"
echo "XAUTH=$XAUTH"

# 5. 創建 X11 認證文件（官方方法）
echo "🔐 創建 X11 認證文件（使用官方方法）..."
# 注意：官方文檔說 "file does not exist" 錯誤是預期的
xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | sudo xauth -f $XAUTH nmerge - 2>/dev/null || echo "⚠️  認證文件創建警告（這是正常的）"
sudo chmod 777 $XAUTH

# 6. 允許 Docker 容器訪問 X11（官方方法）
echo "🔓 允許 Docker 容器訪問 X11..."
xhost +local:docker

# 7. 測試 X11 連接
echo "🧪 測試 X11 連接..."
timeout 5 xeyes >/dev/null 2>&1 && echo "✅ X11 連接測試成功！" || echo "❌ X11 連接測試失敗"

# 8. 停止現有容器
echo "🛑 停止現有容器..."
docker compose down 2>/dev/null || true

# 9. 使用官方方法啟動容器
echo "🚀 使用官方 X11 配置啟動容器..."
DISPLAY=$DISPLAY XSOCK=$XSOCK XAUTH=$XAUTH docker compose up urology-dev -d

# 10. 等待容器啟動
echo "⏳ 等待容器啟動..."
sleep 3

# 11. 在容器內測試 X11
echo "🧪 在容器內測試 X11..."
docker exec -it urology-dev bash -c "
    echo '容器內環境變量：'
    echo 'DISPLAY=' \$DISPLAY
    echo 'XAUTHORITY=' \$XAUTHORITY
    echo 'XSOCK=' \$XSOCK
    echo 'XAUTH=' \$XAUTH
    
    echo '安裝 X11 工具...'
    apt update -qq && apt install -y x11-apps >/dev/null 2>&1
    
    echo '測試 X11 連接...'
    timeout 5 xeyes >/dev/null 2>&1 && echo '✅ 容器內 X11 連接成功！' || echo '❌ 容器內 X11 連接失敗'
"

echo "🎯 準備運行 urology_inference 應用程序..."
echo "如果 X11 測試成功，現在可以運行："
echo "docker exec -it urology-dev bash -c 'cd /workspace/build && ./urology_inference_holoscan_cpp --data=../data'" 