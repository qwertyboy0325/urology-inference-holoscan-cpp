#!/bin/bash

# 一鍵式構建和運行腳本
# 使用預構建的開發容器，無需重新安裝依賴

set -e

echo "🚀 Urology Inference Holoscan C++ - 一鍵式構建和運行"
echo "=================================================="

# 顏色定義
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 檢查 Docker 是否可用
if ! command -v docker &> /dev/null; then
    echo -e "${RED}❌ Docker 未安裝或無法訪問${NC}"
    echo "請先安裝 Docker: https://docs.docker.com/get-docker/"
    exit 1
fi

echo -e "${GREEN}✅ Docker 可用${NC}"

# 檢查數據目錄
if [ ! -d "data" ]; then
    echo -e "${YELLOW}⚠️  創建數據目錄...${NC}"
    mkdir -p data/{models,inputs,output,logs,videos,config}
fi

# 檢查是否有測試視頻
if [ ! -f "data/videos/test.mp4" ]; then
    echo -e "${YELLOW}⚠️  沒有找到測試視頻，將創建一個示例視頻...${NC}"
    docker run --rm -v $(pwd)/data/videos:/output \
        nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu \
        bash -c "
            apt-get update -qq && apt-get install -y -qq ffmpeg
            ffmpeg -f lavfi -i testsrc=duration=10:size=640x480:rate=30 \
                   -c:v libx264 -pix_fmt yuv420p /output/test.mp4
            echo '✅ 測試視頻創建完成'
        "
fi

# 檢查是否有 AI 模型
if [ ! -f "data/models/urology_yolov9c_3000random640resize_20240811_4.34_nhwc.onnx" ]; then
    echo -e "${YELLOW}⚠️  沒有找到 AI 模型文件${NC}"
    echo "請將您的 ONNX 模型文件放置在 data/models/ 目錄中"
    echo "或者我們將使用占位符文件繼續測試..."
    touch data/models/urology_yolov9c_3000random640resize_20240811_4.34_nhwc.onnx
fi

echo -e "${BLUE}🔄 開始一鍵式構建過程（使用預構建容器）...${NC}"

# 首先構建我們的開發容器（如果還沒有構建）
echo -e "${BLUE}🏗️  構建開發容器（包含預安裝的依賴）...${NC}"
docker compose build urology-dev

# 使用我們預構建的開發容器進行構建
echo -e "${BLUE}🔨 使用預構建容器編譯應用程序...${NC}"
docker compose run --rm urology-dev bash -c "
    set -e
    
    echo '🏗️  配置構建環境（依賴已預裝）...'
    rm -rf build
    mkdir -p build
    cd build
    
    echo '⚙️  運行 CMake...'
    cmake .. -DCMAKE_BUILD_TYPE=Release
    
    echo '🔨 編譯應用程序...'
    make -j\$(nproc)
    
    echo '🔧 修復權限...'
    chmod 755 urology_inference_holoscan_cpp
    
    echo '✅ 構建完成！'
    ls -la urology_inference_holoscan_cpp
"

echo -e "${GREEN}🎉 構建成功完成！${NC}"

# 轉換視頻格式（如果需要）
if [ ! -f "data/inputs/tensor.gxf_entities" ] && [ -f "data/videos/test.mp4" ]; then
    echo -e "${BLUE}🔄 轉換視頻格式...${NC}"
    ./scripts/convert_videos.sh data/videos/test.mp4
fi

echo -e "${BLUE}🚀 準備運行應用程序...${NC}"

# 創建運行腳本（使用預構建容器）
cat > run_app.sh << 'EOF'
#!/bin/bash

# 運行應用程序（使用預構建容器）
echo "🚀 啟動 Urology Inference 應用程序（依賴已預裝）..."

# 使用我們的預構建開發容器運行
docker compose run --rm urology-dev bash -c "
    echo '🎯 運行 Urology Inference（依賴已預裝）...'
    cd /workspace/build
    ./urology_inference_holoscan_cpp --data=../data
"
EOF

chmod +x run_app.sh

echo ""
echo -e "${GREEN}🎉 一鍵式構建完成！${NC}"
echo ""
echo -e "${BLUE}📋 接下來您可以：${NC}"
echo ""
echo -e "${YELLOW}1. 運行應用程序：${NC}"
echo "   ./run_app.sh"
echo ""
echo -e "${YELLOW}2. 查看構建的文件：${NC}"
echo "   ls -la build/"
echo ""
echo -e "${YELLOW}3. 檢查數據目錄：${NC}"
echo "   ls -la data/"
echo ""
echo -e "${YELLOW}4. 直接使用 Docker Compose：${NC}"
echo "   docker compose run --rm urology-dev"
echo ""
echo -e "${GREEN}✨ 現在您可以直接運行 ./run_app.sh 來啟動應用程序！${NC}"
echo -e "${GREEN}💡 所有依賴都已預安裝在容器中，無需重新安裝！${NC}" 