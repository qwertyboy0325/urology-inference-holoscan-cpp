#!/bin/bash

# 修復容器內 X11 認證問題的腳本
# 適用於 SSH X11 轉發環境

echo "🔧 修復容器內 X11 認證..."

# 1. 檢查當前環境
echo "📋 當前環境："
echo "DISPLAY=$DISPLAY"
echo "XAUTHORITY=$XAUTHORITY"

# 2. 安裝必要工具
echo "📦 安裝 X11 工具..."
apt update -qq && apt install -y x11-apps xauth

# 3. 檢查 X11 socket
echo "🔍 檢查 X11 socket："
ls -la /tmp/.X11-unix/ 2>/dev/null || echo "❌ X11 socket 不存在"

# 4. 修復認證文件權限
if [ -f "$XAUTHORITY" ]; then
    echo "🔑 修復認證文件權限："
    chmod 644 $XAUTHORITY
    ls -la $XAUTHORITY
else
    echo "⚠️  XAUTHORITY 文件不存在：$XAUTHORITY"
fi

# 5. 嘗試從主機複製認證
echo "🔄 嘗試從主機獲取 X11 認證..."
if [ -n "$DISPLAY" ]; then
    # 方法 1：嘗試使用 xauth 提取認證
    echo "方法 1：使用 xauth 提取認證..."
    xauth extract - $DISPLAY 2>/dev/null | xauth merge - 2>/dev/null || echo "❌ xauth 提取失敗"
    
    # 方法 2：手動設置認證
    echo "方法 2：手動設置認證..."
    DISPLAY_NUM=$(echo $DISPLAY | sed 's/localhost://' | sed 's/:.*//')
    echo "顯示編號：$DISPLAY_NUM"
    
    # 嘗試不同的認證方法
    xauth add $DISPLAY . $(mcookie) 2>/dev/null || echo "❌ 手動認證設置失敗"
fi

# 6. 測試 X11 連接
echo "🧪 測試 X11 連接..."
echo "嘗試 1：直接測試 xeyes"
timeout 3 xeyes >/dev/null 2>&1 && echo "✅ X11 連接成功！" || echo "❌ 第一次測試失敗"

# 7. 如果失敗，嘗試替代方法
if ! timeout 3 xeyes >/dev/null 2>&1; then
    echo "🔄 嘗試替代方法..."
    
    # 方法 A：重設 DISPLAY
    for display in "localhost:10.0" "localhost:11.0" ":10.0" ":11.0" ":0"; do
        echo "測試 DISPLAY=$display"
        export DISPLAY=$display
        if timeout 2 xdpyinfo >/dev/null 2>&1; then
            echo "✅ 找到可用顯示：$display"
            timeout 3 xeyes >/dev/null 2>&1 && echo "✅ X11 連接成功！" && break
        fi
    done
    
    # 方法 B：使用 SSH 轉發的特殊設置
    echo "🔄 嘗試 SSH 轉發特殊設置..."
    export DISPLAY=localhost:10.0
    unset XAUTHORITY
    timeout 3 xeyes >/dev/null 2>&1 && echo "✅ 無認證文件連接成功！" || echo "❌ 仍然失敗"
fi

echo "🎯 X11 修復完成！現在可以測試 Holoscan 應用程序" 