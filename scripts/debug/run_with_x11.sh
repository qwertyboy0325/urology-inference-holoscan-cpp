#!/bin/bash

# X11 轉發設置腳本
# 適用於 SSH X11 轉發環境

set -e

echo "🖥️  設置 X11 轉發環境..."

# 檢查 DISPLAY 環境變量
if [ -z "$DISPLAY" ]; then
    echo "❌ DISPLAY 環境變量未設置"
    echo "請確保你的 SSH 連接啟用了 X11 轉發："
    echo "ssh -X username@hostname"
    exit 1
fi

echo "✅ DISPLAY: $DISPLAY"

# 創建 X11 認證文件
XAUTH_FILE="/tmp/.docker.xauth"
echo "🔑 創建 X11 認證文件: $XAUTH_FILE"

# 如果 xauth 命令存在，創建認證文件
if command -v xauth >/dev/null 2>&1; then
    xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f $XAUTH_FILE nmerge -
    chmod 644 $XAUTH_FILE
    echo "✅ X11 認證文件創建成功"
else
    echo "⚠️  xauth 命令不存在，創建空認證文件"
    touch $XAUTH_FILE
    chmod 644 $XAUTH_FILE
fi

# 設置環境變量
export XAUTHORITY=$XAUTH_FILE

# 停止現有容器
echo "🛑 停止現有容器..."
docker compose down 2>/dev/null || true

# 啟動容器
echo "🚀 啟動支持 X11 的容器..."
DISPLAY=$DISPLAY XAUTHORITY=$XAUTH_FILE docker compose up urology-dev -d

# 等待容器啟動
echo "⏳ 等待容器啟動..."
sleep 3

# 在容器內安裝 X11 工具並測試
echo "🔧 在容器內安裝 X11 工具..."
docker exec -it urology-dev bash -c "
    apt update -qq && apt install -y x11-apps xauth >/dev/null 2>&1
    echo '✅ X11 工具安裝完成'
    
    echo '🧪 測試 X11 連接...'
    echo 'DISPLAY=' \$DISPLAY
    echo 'XAUTHORITY=' \$XAUTHORITY
    
    # 測試 X11 連接
    timeout 5 xeyes >/dev/null 2>&1 && echo '✅ X11 連接測試成功' || echo '❌ X11 連接測試失敗'
"

echo "🎯 運行 urology_inference 應用程序..."
docker exec -it urology-dev bash -c "
    cd /workspace/build && ./urology_inference_holoscan_cpp --data=../data
" 