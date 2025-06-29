#!/bin/bash

# Docker Quick Start Script for Urology Inference Holoscan C++
# This script helps users get started with Docker quickly

set -e

# Color codes
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

show_header() {
    echo -e "${GREEN}🐳 Urology Inference Holoscan C++ - Docker Quick Start${NC}"
    echo -e "${BLUE}基於 NVIDIA Holoscan SDK 3.3.0${NC}"
    echo "================================================"
    echo ""
}

check_docker() {
    log_info "檢查 Docker 安裝..."
    
    if ! command -v docker &> /dev/null; then
        log_error "Docker 未安裝！請先安裝 Docker"
        echo "安裝指南: https://docs.docker.com/get-docker/"
        exit 1
    fi
    
    if ! command -v docker-compose &> /dev/null; then
        log_error "Docker Compose 未安裝！請先安裝 Docker Compose"
        echo "安裝指南: https://docs.docker.com/compose/install/"
        exit 1
    fi
    
    log_success "Docker 環境檢查通過"
}

check_nvidia_docker() {
    log_info "檢查 NVIDIA Docker 支持..."
    
    if ! docker info 2>/dev/null | grep -q nvidia; then
        log_warning "NVIDIA Docker 運行時未安裝或配置"
        log_warning "GPU 功能可能不可用"
        echo ""
        echo "安裝 NVIDIA Container Toolkit:"
        echo "  https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/install-guide.html"
        echo ""
        read -p "是否繼續？ (y/N) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    else
        log_success "NVIDIA Docker 運行時可用"
    fi
}

show_quick_start_menu() {
    echo -e "${GREEN}請選擇要執行的操作：${NC}"
    echo ""
    echo "1) 🚀 運行生產環境 (推薦新用戶)"
    echo "2) 🛠️  進入開發環境"
    echo "3) 🧪 運行測試"
    echo "4) 📊 運行性能基準測試"
    echo "5) 🔍 驗證視頻編碼器依賴"
    echo "6) 🔧 構建 Docker 鏡像"
    echo "7) 📚 查看使用說明"
    echo "8) 🚪 退出"
    echo ""
}

run_production() {
    log_info "啟動生產環境..."
    
    # Check if images exist
    if ! docker images | grep -q "urology-inference.*runtime"; then
        log_info "未發現運行時鏡像，開始構建..."
        ./scripts/docker-build.sh --runtime --release
    fi
    
    log_info "使用 Docker Compose 啟動服務..."
    docker-compose up urology-inference
}

run_development() {
    log_info "啟動開發環境..."
    
    # Check if dev images exist
    if ! docker images | grep -q "urology-inference.*development"; then
        log_info "未發現開發鏡像，開始構建..."
        ./scripts/docker-build.sh --development --debug
    fi
    
    log_info "進入開發模式容器..."
    docker-compose --profile development run --rm urology-inference-dev
}

run_tests() {
    log_info "運行測試套件..."
    
    if ! docker images | grep -q "urology-inference.*runtime"; then
        log_info "未發現運行時鏡像，開始構建..."
        ./scripts/docker-build.sh --runtime --release
    fi
    
    docker-compose --profile testing up urology-inference-test
}

run_benchmarks() {
    log_info "運行性能基準測試..."
    
    if ! docker images | grep -q "urology-inference.*runtime"; then
        log_info "未發現運行時鏡像，開始構建..."
        ./scripts/docker-build.sh --runtime --release
    fi
    
    docker-compose --profile benchmarking up urology-inference-benchmark
}

verify_dependencies() {
    log_info "驗證視頻編碼器依賴..."
    
    if ! docker images | grep -q "urology-inference.*runtime"; then
        log_info "未發現運行時鏡像，開始構建..."
        ./scripts/docker-build.sh --runtime --release
    fi
    
    log_info "運行依賴驗證..."
    docker run --rm --gpus all urology-inference:runtime-latest verify-deps
    
    read -p "是否運行詳細驗證？ (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        log_info "運行詳細依賴驗證..."
        docker run --rm --gpus all urology-inference:runtime-latest verify-deps --verbose
    fi
}

build_images() {
    echo -e "${GREEN}選擇要構建的鏡像類型：${NC}"
    echo "1) 運行時鏡像 (生產用)"
    echo "2) 開發鏡像"
    echo "3) 所有鏡像"
    echo ""
    read -p "請選擇 (1-3): " build_choice
    
    case $build_choice in
        1)
            ./scripts/docker-build.sh --runtime --release
            ;;
        2)
            ./scripts/docker-build.sh --development --debug
            ;;
        3)
            ./scripts/docker-build.sh --all
            ;;
        *)
            log_error "無效選擇"
            return 1
            ;;
    esac
}

show_documentation() {
    echo -e "${GREEN}📚 文檔和資源：${NC}"
    echo ""
    echo "• 主要 README: README.md"
    echo "• Docker 詳細指南: DOCKER_README.md"
    echo "• 優化總結: OPTIMIZATION_SUMMARY.md"
    echo ""
    echo -e "${GREEN}常用 Docker 命令：${NC}"
    echo ""
    echo "# 查看運行中的容器"
    echo "docker ps"
    echo ""
    echo "# 查看所有鏡像"
    echo "docker images"
    echo ""
    echo "# 進入運行中的容器"
    echo "docker exec -it <container_name> bash"
    echo ""
    echo "# 查看容器日誌"
    echo "docker-compose logs -f urology-inference"
    echo ""
    echo "# 清理未使用的資源"
    echo "docker system prune"
    echo ""
}

main() {
    show_header
    check_docker
    check_nvidia_docker
    
    while true; do
        echo ""
        show_quick_start_menu
        read -p "請選擇 (1-8): " choice
        
        case $choice in
            1)
                run_production
                ;;
            2)
                run_development
                ;;
            3)
                run_tests
                ;;
            4)
                run_benchmarks
                ;;
            5)
                verify_dependencies
                ;;
            6)
                build_images
                ;;
            7)
                show_documentation
                ;;
            8)
                log_info "感謝使用 Urology Inference Holoscan C++!"
                exit 0
                ;;
            *)
                log_error "無效選擇，請輸入 1-8"
                ;;
        esac
        
        echo ""
        read -p "按 Enter 繼續..."
    done
}

# Run main function
main "$@" 