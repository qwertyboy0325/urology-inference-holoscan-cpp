#!/bin/bash

# 🎯 快速測試修復後的應用程序

set -e

echo "🔨 快速重新構建應用程序..."

# 檢查是否在容器中
if [ -f /.dockerenv ]; then
    echo "✅ 在容器中運行"
    
    # 重新構建
    cd /workspace/build
    make -j$(nproc)
    
    echo "✅ 構建完成！"
    echo ""
    echo "🎯 測試修復後的應用程序..."
    echo ""
    
    # 測試 UI 模式
    echo "🖥️  測試 UI 模式："
    echo "./urology_inference_holoscan_cpp --data=../data"
    echo ""
    
    # 測試 Headless 模式  
    echo "🖥️  測試 Headless 模式："
    echo "HOLOVIZ_HEADLESS=1 ./urology_inference_holoscan_cpp --data=../data"
    echo ""
    
    echo "請選擇測試模式："
    echo "1) UI 模式"
    echo "2) Headless 模式"
    echo "3) 進入交互模式"
    read -p "選擇 [1/2/3]: " choice
    
    case $choice in
        1)
            echo "🎯 運行 UI 模式..."
            ./urology_inference_holoscan_cpp --data=../data
            ;;
        2)
            echo "🎯 運行 Headless 模式..."
            HOLOVIZ_HEADLESS=1 ./urology_inference_holoscan_cpp --data=../data
            ;;
        3)
            echo "🎯 進入交互模式..."
            /bin/bash
            ;;
        *)
            echo "🎯 默認進入交互模式..."
            /bin/bash
            ;;
    esac
    
else
    echo "❌ 不在容器中，請先啟動開發環境"
    echo "使用以下命令之一："
    echo "./scripts/setup_and_run_x11.sh"
    echo "或"
    echo "./scripts/run_with_x11_fixed.sh"
fi 