# 🐳 Urology Inference Holoscan C++ - Docker 部署指南

本指南說明如何使用 Docker 容器運行基於 NVIDIA Holoscan SDK 3.3.0 的 Urology Inference 應用程序。

## 📋 前置需求

### 硬件需求
- **GPU**: NVIDIA GPU (推薦 RTX 30系列或更新)
- **內存**: 最少 8GB RAM
- **存儲**: 最少 10GB 可用空間

### 軟件需求
- **Docker**: 20.10+ 
- **Docker Compose**: 2.0+
- **NVIDIA Container Toolkit**: 最新版本
- **NVIDIA 驅動**: 525+ (支援 CUDA 12.0+)

### 安裝 NVIDIA Container Toolkit

```bash
# Ubuntu/Debian
distribution=$(. /etc/os-release;echo $ID$VERSION_ID)
curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -
curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | sudo tee /etc/apt/sources.list.d/nvidia-docker.list

sudo apt-get update && sudo apt-get install -y nvidia-container-toolkit
sudo systemctl restart docker
```

## 🚀 快速開始

### 1. 使用 Docker Compose (推薦)

```bash
# 克隆項目
git clone <repository>
cd urology-inference-holoscan-cpp

# 運行生產環境
docker-compose up urology-inference

# 運行開發環境
docker-compose --profile development up urology-inference-dev
```

### 2. 使用構建腳本

```bash
# 構建運行時鏡像
./scripts/docker-build.sh --runtime --release

# 構建開發鏡像
./scripts/docker-build.sh --development --debug

# 構建所有鏡像
./scripts/docker-build.sh --all
```

### 3. 直接使用 Docker

```bash
# 構建鏡像
docker build -t urology-inference:runtime --target runtime .

# 運行容器
docker run --gpus all -it urology-inference:runtime --help
```

## 🏗️ 多階段構建架構

### Runtime Stage (生產環境)
```dockerfile
FROM nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu
```
- 最小化的運行時環境
- 只包含必要的運行時依賴
- 優化的應用程序二進制文件
- 鏡像大小: ~2-3GB

### Development Stage (開發環境)
```dockerfile
FROM nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu
```
- 完整的開發工具鏈
- 包含調試工具和分析器
- 支持實時代碼編輯
- 鏡像大小: ~5-6GB

## 📊 Docker Compose 服務

### 生產服務 (`urology-inference`)
```yaml
# 運行生產應用
docker-compose up urology-inference
```

**特性**:
- GPU 加速推理
- 持久化日誌和輸出
- 健康檢查
- 資源限制
- 安全配置

### 開發服務 (`urology-inference-dev`)
```yaml
# 進入開發模式
docker-compose --profile development up urology-inference-dev
```

**特性**:
- 源代碼卷掛載
- 交互式 shell
- 調試工具支持
- 構建緩存

### 測試服務 (`urology-inference-test`)
```yaml
# 運行測試
docker-compose --profile testing up urology-inference-test
```

### 基準測試服務 (`urology-inference-benchmark`)
```yaml
# 運行性能基準測試
docker-compose --profile benchmarking up urology-inference-benchmark
```

## 🔧 配置和卷掛載

### 環境變數
```bash
# 核心路徑配置
HOLOHUB_DATA_PATH=/app/data
HOLOSCAN_MODEL_PATH=/app/models
UROLOGY_CONFIG_PATH=/app/config
UROLOGY_LOG_PATH=/app/logs
UROLOGY_OUTPUT_PATH=/app/output

# 應用配置
UROLOGY_RECORD_OUTPUT=true
UROLOGY_INFERENCE_BACKEND=trt

# GPU 配置
NVIDIA_VISIBLE_DEVICES=all
CUDA_VISIBLE_DEVICES=all
```

### 卷掛載配置
```yaml
volumes:
  # 輸入數據 (只讀)
  - ./data:/app/data:ro
  - ./models:/app/models:ro
  - ./config:/app/config:ro
  
  # 輸出數據 (讀寫)
  - urology_logs:/app/logs
  - urology_output:/app/output
```

### 目錄結構
```
urology-inference-holoscan-cpp/
├── data/                    # 輸入數據
│   ├── inputs/             # 視頻/圖像輸入
│   └── models/             # 推理模型
├── config/                 # 配置文件
│   ├── app_config.yaml
│   └── labels.yaml
├── models/                 # 額外模型文件
└── logs/                   # 應用日誌 (容器創建)
```

## 🎮 使用指南

### 基本命令

```bash
# 查看幫助
docker run --gpus all urology-inference:runtime help

# 檢查環境
docker run --gpus all urology-inference:runtime env

# 查看版本
docker run --gpus all urology-inference:runtime version

# 檢查依賴
docker run --gpus all urology-inference:runtime verify-deps

# 運行推理
docker run --gpus all \
  -v $(pwd)/data:/app/data:ro \
  -v $(pwd)/models:/app/models:ro \
  urology-inference:runtime \
  --source replayer \
  --record_output true
```

### 開發模式

```bash
# 進入開發容器
docker-compose --profile development run --rm urology-inference-dev

# 在容器內編譯
./scripts/build_optimized.sh --debug --enable-testing

# 運行測試
cd build && ctest --output-on-failure
```

### 測試和基準測試

```bash
# 運行單元測試
docker run --gpus all urology-inference:runtime test

# 運行性能基準測試
docker run --gpus all urology-inference:runtime benchmark

# 查看測試結果
docker run --gpus all \
  -v urology_test_results:/results \
  urology-inference:runtime \
  cat /results/test_report.xml
```

## 📊 監控和日誌

### 日誌訪問
```bash
# 查看實時日誌
docker-compose logs -f urology-inference

# 查看容器內日誌
docker exec -it urology-inference-runtime tail -f /app/logs/urology_*.log
```

### 性能監控
```bash
# GPU 使用情況
docker exec -it urology-inference-runtime nvidia-smi

# 容器資源使用
docker stats urology-inference-runtime

# 應用性能指標
docker exec -it urology-inference-runtime \
  ./urology_inference_holoscan_cpp --monitor-performance
```

### 健康檢查
```bash
# 檢查容器健康狀態
docker inspect --format='{{.State.Health.Status}}' urology-inference-runtime

# 查看健康檢查日誌
docker inspect --format='{{range .State.Health.Log}}{{.Output}}{{end}}' urology-inference-runtime
```

## 🔍 依賴驗證

### 視頻編碼器依賴檢查

項目包含完整的視頻編碼器依賴驗證機制，確保所有 GXF 多媒體擴展正確安裝：

```bash
# 在容器中驗證依賴
docker run --gpus all urology-inference:runtime verify-deps

# 或使用 Docker Compose
docker-compose run --rm urology-inference verify-deps

# 詳細驗證信息
docker run --gpus all urology-inference:runtime verify-deps --verbose
```

### 自動驗證

- **構建時驗證**: Docker 構建過程中自動安裝和驗證依賴
- **啟動時檢查**: 容器啟動時自動檢查關鍵依賴
- **健康檢查**: Docker Compose 健康檢查包含依賴驗證

### 驗證內容

✅ **GXF 多媒體擴展**
- libgxf_encoder.so (H.264/H.265 編碼)
- libgxf_encoderio.so (編碼器 I/O 操作)
- libgxf_decoder.so (視頻解碼)
- libgxf_decoderio.so (解碼器 I/O 操作)

✅ **系統依賴**
- NVIDIA 驅動檢查
- CUDA 運行時檢查
- Holoscan SDK 版本驗證

✅ **庫依賴**
- 動態鏈接庫完整性
- 權限和可讀性檢查
- 文件大小驗證

### 手動驗證

如果需要在主機系統上驗證：

```bash
# 直接運行驗證腳本
./scripts/verify_video_encoder_deps.sh

# 詳細模式
./scripts/verify_video_encoder_deps.sh --verbose

# 自定義庫路徑
./scripts/verify_video_encoder_deps.sh --libs-dir /custom/path/to/holoscan/lib
```

## 🔧 故障排除

### 常見問題

**1. GPU 不可用**
```bash
# 檢查 NVIDIA 運行時
docker info | grep nvidia

# 測試 GPU 訪問
docker run --gpus all nvidia/cuda:12.0-runtime-ubuntu20.04 nvidia-smi
```

**2. 內存不足**
```bash
# 增加 Docker 內存限制
docker-compose up --memory=8g urology-inference
```

**3. 權限問題**
```bash
# 檢查卷掛載權限
ls -la data/ models/ config/

# 修復權限
sudo chown -R $USER:$USER data/ models/ config/
```

### 調試模式

```bash
# 進入容器 shell
docker run --gpus all -it urology-inference:runtime shell

# 運行調試版本
docker-compose --profile development run --rm urology-inference-dev gdb ./build/urology_inference_holoscan_cpp
```

### 日誌分析

```bash
# 檢查構建日誌
docker-compose build --no-cache urology-inference 2>&1 | tee build.log

# 分析運行時錯誤
docker-compose logs urology-inference | grep ERROR

# 導出性能日誌
docker cp urology-inference-runtime:/app/logs ./exported_logs/
```

## 🚀 生產部署

### 資源配置
```yaml
# docker-compose.prod.yml
services:
  urology-inference:
    deploy:
      resources:
        limits:
          memory: 16G
          cpus: '8'
        reservations:
          memory: 8G
          devices:
            - driver: nvidia
              count: 1
              capabilities: [gpu, compute, video]
```

### 安全配置
```yaml
security_opt:
  - no-new-privileges:true
  - seccomp:unconfined  # 如果需要特殊系統調用
read_only: true
tmpfs:
  - /tmp:size=1G
user: 1001:1001
```

### 持久化存儲
```yaml
volumes:
  urology_data:
    driver: local
    driver_opts:
      type: none
      o: bind
      device: /storage/urology-inference
```

## 📈 性能優化

### 構建優化
```bash
# 使用 BuildKit
export DOCKER_BUILDKIT=1

# 多階段緩存
docker build --cache-from urology-inference:builder --target runtime .

# 並行構建
docker buildx build --platform linux/amd64 --load .
```

### 運行時優化
```yaml
# 減少啟動時間
environment:
  - NVIDIA_DRIVER_CAPABILITIES=compute,video
  - CUDA_CACHE_DISABLE=0

# 優化內存使用
shm_size: 2g
ulimits:
  memlock:
    soft: -1
    hard: -1
```

## 📝 最佳實踐

1. **鏡像管理**
   - 使用特定版本標籤
   - 定期清理未使用的鏡像
   - 實施鏡像掃描

2. **數據管理**
   - 使用命名卷進行持久化
   - 定期備份重要數據
   - 監控存儲使用情況

3. **安全**
   - 不以 root 用戶運行
   - 使用只讀根文件系統
   - 限制容器能力

4. **監控**
   - 實施健康檢查
   - 監控資源使用
   - 設置日誌輪換

## 🔗 相關資源

- [NVIDIA Holoscan Documentation](https://docs.nvidia.com/holoscan/)
- [Docker GPU Support](https://docs.docker.com/config/containers/resource_constraints/#gpu)
- [NVIDIA Container Toolkit](https://github.com/NVIDIA/nvidia-container-toolkit)
- [Docker Compose GPU](https://docs.docker.com/compose/gpu-support/)

---

**維護者**: Urology Inference Team  
**版本**: 1.0.0  
**基於**: NVIDIA Holoscan SDK 3.3.0 