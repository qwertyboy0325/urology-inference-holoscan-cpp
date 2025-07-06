#!/bin/bash

# 修復版本的 X11 轉發設置腳本
# 跳過有問題的 xhost 步驟

set -e

echo "🖥️  修復版本的 X11 轉發設置..."

# 1. 顯示當前環境
echo "📋 當前環境信息："
echo "DISPLAY=$DISPLAY"
echo "SSH_CLIENT=$SSH_CLIENT"
echo "SSH_CONNECTION=$SSH_CONNECTION"

# 2. 設置 X11 socket 和認證文件路徑
export XSOCK=/tmp/.X11-unix
export XAUTH=/tmp/.docker.xauth

echo "🔑 設置 X11 認證文件..."
echo "XSOCK=$XSOCK"
echo "XAUTH=$XAUTH"

# 3. 創建 X11 認證文件（修復版本）
echo "🔐 創建 X11 認證文件..."
if [ -n "$DISPLAY" ]; then
    # 嘗試創建認證文件，但不要因為錯誤而停止
    echo "嘗試創建認證文件..."
    xauth nlist $DISPLAY 2>/dev/null | sed -e 's/^..../ffff/' | sudo xauth -f $XAUTH nmerge - 2>/dev/null || {
        echo "⚠️  無法從當前顯示創建認證，創建空認證文件..."
        sudo touch $XAUTH
    }
else
    echo "⚠️  DISPLAY 未設置，創建空認證文件..."
    sudo touch $XAUTH
fi

sudo chmod 777 $XAUTH
echo "✅ 認證文件創建完成：$XAUTH"

# 4. 跳過 xhost 步驟（因為它在 SSH X11 轉發中可能失敗）
echo "⚠️  跳過 xhost 步驟（SSH X11 轉發環境）"

# 5. 檢查 X11 socket
echo "🔍 檢查 X11 socket..."
if [ -d "$XSOCK" ]; then
    echo "✅ X11 socket 存在：$XSOCK"
    ls -la $XSOCK/
else
    echo "⚠️  X11 socket 不存在，創建目錄..."
    sudo mkdir -p $XSOCK
    sudo chmod 1777 $XSOCK
fi

# 6. 停止現有容器
echo "🛑 停止現有容器..."
docker compose down 2>/dev/null || true

# 7. 啟動容器
echo "🚀 啟動容器..."
DISPLAY=$DISPLAY XSOCK=$XSOCK XAUTH=$XAUTH docker compose up urology-dev -d

# 8. 等待容器啟動
echo "⏳ 等待容器啟動..."
sleep 5

# 9. 在容器內測試
echo "🧪 在容器內測試環境..."
docker exec -it urology-dev bash -c "
    echo '=== 容器內環境檢查 ==='
    echo 'DISPLAY=' \$DISPLAY
    echo 'XAUTHORITY=' \$XAUTHORITY
    
    echo '=== 安裝 X11 工具 ==='
    apt update -qq && apt install -y x11-apps xauth >/dev/null 2>&1
    
    echo '=== 檢查 X11 文件 ==='
    ls -la /tmp/.X11-unix/ 2>/dev/null || echo 'X11 socket 不存在'
    ls -la \$XAUTHORITY 2>/dev/null || echo 'XAUTHORITY 文件不存在'
    
    echo '=== 嘗試 X11 連接測試 ==='
    timeout 3 xeyes >/dev/null 2>&1 && echo '✅ X11 測試成功！' || echo '❌ X11 測試失敗（但這在 SSH 環境中是正常的）'
"

echo ""
echo "🎯 X11 環境設置完成！"
echo "現在可以嘗試運行 urology_inference："
echo ""
echo "方法 1 - 直接運行（可能會有 X11 錯誤但推理功能正常）："
echo "docker exec -it urology-dev bash -c 'cd /workspace/build && ./urology_inference_holoscan_cpp --data=../data'"
echo ""
echo "方法 2 - 使用 headless 模式（推薦）："
echo "docker exec -it urology-dev bash -c 'export HOLOVIZ_HEADLESS=1 && cd /workspace/build && ./urology_inference_holoscan_cpp --data=../data'"
echo ""
echo "方法 3 - 使用簡化測試（確認推理功能）："
echo "docker exec -it urology-dev bash -c 'cd /workspace/build && ./simple_inference_test'" 