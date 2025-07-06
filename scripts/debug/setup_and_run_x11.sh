#!/bin/bash

# 🎯 設置 X11 認證並啟動 Urology Inference 開發環境
# 整合成功的 X11 解決方案到原本的 Docker 環境

set -e

echo "🚀 設置 X11 認證並啟動 Urology Inference 開發環境..."

# 1. 檢查 SSH X11 轉發環境
echo "📋 檢查 SSH X11 轉發環境..."
echo "DISPLAY: $DISPLAY"
echo "SSH_CLIENT: $SSH_CLIENT"

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
docker compose -f docker-compose.x11.yml down 2>/dev/null || true

# 7. 構建開發鏡像（如果需要）
echo "🔨 構建開發鏡像..."
docker compose -f docker-compose.x11.yml build urology-dev-x11

# 8. 啟動開發環境
echo "🚀 啟動開發環境..."
echo "使用以下命令："
echo "DISPLAY=$DISPLAY XAUTH=$XAUTH docker compose -f docker-compose.x11.yml up urology-dev-x11"
echo ""

# 導出環境變數並啟動
export DISPLAY=$DISPLAY
export XAUTH=$XAUTH
docker compose -f docker-compose.x11.yml up urology-dev-x11 