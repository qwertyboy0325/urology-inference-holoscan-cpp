# Urology Inference Holoscan C++

🚀 **一鍵式醫學影像分析系統** - 基於 NVIDIA Holoscan SDK 的泌尿科推理應用程序

## ✨ 特點

- 🎯 **一鍵運行**：無需手動安裝依賴，一個命令搞定所有
- 🔥 **GPU 加速**：支援 NVIDIA GPU 加速推理
- 📹 **視頻處理**：自動處理視頻格式轉換
- 🐳 **容器化**：完全基於 Docker，環境一致性保證
- 🛠️ **OpenCV 集成**：完整的圖像處理功能

## 🚀 快速開始

### 方法一：使用 Make（推薦）

```bash
# 完整流程：設置 + 構建 + 測試
make all

# 運行應用程序
make run
```

### 方法二：使用腳本

```bash
# 一鍵構建
./scripts/build_and_run.sh

# 運行應用程序
./run_app.sh
```

### 方法三：使用 Docker Compose

```bash
# 構建
docker compose run --rm urology-dev

# 運行
docker compose run --rm urology-run
```

## 📋 可用命令

| 命令 | 說明 |
|------|------|
| `make all` | 完整流程：設置 + 構建 + 測試 |
| `make build` | 一鍵式構建應用程序 |
| `make run` | 運行應用程序 |
| `make test` | 運行快速測試 |
| `make dev` | 啟動開發環境 |
| `make clean` | 清理構建文件 |
| `make info` | 查看項目信息 |
| `make logs` | 查看應用程序日誌 |

## 📁 項目結構

```
urology-inference-holoscan-cpp/
├── data/                   # 數據目錄
│   ├── models/            # AI 模型文件 (ONNX)
│   ├── inputs/            # 輸入視頻 (GXF 格式)
│   ├── videos/            # 原始視頻文件
│   ├── output/            # 輸出結果
│   ├── logs/              # 應用程序日誌
│   └── config/            # 配置文件
├── src/                   # 源代碼
│   ├── operators/         # 自定義操作符
│   ├── utils/             # 工具函數
│   └── config/            # 配置管理
├── include/               # 頭文件
├── scripts/               # 核心腳本
├── CMakeLists.txt         # 構建配置
├── Makefile              # 簡化命令
├── Dockerfile            # 容器構建
├── docker-compose.yml    # 容器編排
└── metadata.json         # HoloHub 元數據
```

## 🔧 系統要求

- **Docker**：版本 20.10+
- **NVIDIA Container Toolkit**：用於 GPU 支持
- **Docker Compose**：用於容器編排

⚠️ **重要**：此系統完全在容器內運行，無需在主機系統安裝 Holoscan SDK 或 OpenCV

### 🐳 容器化優勢

- **環境隔離**：不會影響主機系統
- **依賴管理**：所有依賴都在容器內自動安裝
- **版本一致**：確保所有開發者使用相同的環境
- **跨平台**：在任何支持 Docker 的系統上運行

## 📦 預裝的依賴（Dockerfile 中預裝）

所有依賴都在 Docker 鏡像構建時預先安裝，啟動容器時無需重新安裝：

- ✅ **NVIDIA Holoscan SDK 3.3.0**：完整的醫學影像處理框架
- ✅ **OpenCV 4.6.0**：完整的圖像處理和電腦視覺庫（所有模組）
- ✅ **CUDA/TensorRT**：GPU 加速推理
- ✅ **FFmpeg**：完整的視頻編碼和解碼支持
- ✅ **X11 GUI 支持**：圖形界面顯示（libgtk-3-dev 等）
- ✅ **CMake + Ninja**：高效構建系統
- ✅ **開發工具**：GDB、Valgrind、Clang-tidy 等

⚡ **優勢**：
- 🚀 **快速啟動**：無需等待依賴安裝
- 🔒 **版本鎖定**：確保環境一致性
- 💾 **節省頻寬**：避免重複下載

## 🎯 使用流程

### 1. 初次使用

```bash
# 克隆項目
git clone <repository-url>
cd urology-inference-holoscan-cpp

# 一鍵完成所有設置
make all
```

### 2. 日常使用

```bash
# 運行應用程序
make run

# 或者查看幫助
make help
```

### 3. 開發模式

```bash
# 啟動開發環境
make dev

# 在容器內進行開發
# 修改代碼後重新構建
make build
```

## 📊 測試和驗證

```bash
# 運行快速測試
make test

# 查看構建信息
make info

# 查看日誌
make logs
```

## 🐛 故障排除

### 常見問題

1. **Docker 權限問題**
   ```bash
   sudo usermod -aG docker $USER
   # 重新登錄後生效
   ```

2. **GPU 支援問題**
   ```bash
   # 檢查 NVIDIA Docker
   docker run --rm --gpus all nvidia/cuda:11.0-base nvidia-smi
   ```

3. **構建失敗**
   ```bash
   # 清理並重新構建
   make clean
   make build
   ```

### 查看詳細錯誤

```bash
# 查看構建日誌
make build 2>&1 | tee build.log

# 查看運行日誌
make logs
```

## 🔄 更新和維護

```bash
# 清理舊文件
make clean

# 重新構建
make build

# 更新 Docker 鏡像
docker pull nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu
```

## 📚 更多信息

- [NVIDIA Holoscan SDK 文檔](https://docs.nvidia.com/holoscan/)
- [OpenCV 文檔](https://docs.opencv.org/)
- [Docker 文檔](https://docs.docker.com/)

## 🤝 貢獻

歡迎提交 Issues 和 Pull Requests！

## 📄 許可證

本項目採用 [MIT 許可證](LICENSE)

---

💡 **提示**：如果您是第一次使用，建議直接運行 `make all` 來體驗完整的一鍵式流程！

## 運行應用程序

### 基本運行
```bash
# 使用 Makefile（推薦）
make run

# 或直接使用腳本
./scripts/run.sh
```

### Headless 模式（無顯示視窗）

**重要**: HolovizOp 的 headless 模式通過 `enable_render_buffer_output=true` 實現，這樣可以在無顯示環境下運行，同時保持完整的渲染管道功能。

```bash
# 使用 Makefile
make run-headless

# 或使用腳本參數
./scripts/run.sh --headless

# 或設置環境變量
export HOLOVIZ_HEADLESS=1
./scripts/run.sh
```

### 其他運行選項
```bash
# 指定數據路徑
./scripts/run.sh --data /path/to/data

# 指定源類型
./scripts/run.sh --source yuan

# 設置日誌級別
./scripts/run.sh --log-level DEBUG

# 組合使用
./scripts/run.sh --headless --log-level INFO --data ./data
```

## 🎯 功能特色

### 🎯 核心功能
- **實時視頻推理**: 使用 YOLO 模型進行泌尿科手術工具檢測和分割
- **多源輸入支持**: 支持 GXF 格式的視頻回放和實時視頻流
- **高性能處理**: 基於 CUDA 和 TensorRT 的 GPU 加速推理
- **可視化輸出**: 使用 HolovizOp 進行實時結果可視化

### 🖥️ Headless 模式支持
- **無顯示運行**: 支持在無 GUI 環境下運行，適合服務器部署
- **渲染緩衝輸出**: HolovizOp 在 headless 模式下輸出渲染緩衝數據
- **靈活部署**: 支持容器化和遠程部署場景

### 🐳 Docker 集成
- **一鍵構建**: 使用 `make build` 或 `scripts/build_and_run.sh` 一鍵構建
- **優化的容器配置**: 基於 NVIDIA Holoscan 官方容器
- **OpenCV 集成**: 完整的 OpenCV 4.6.0 支持

## 📚 更多信息

- [NVIDIA Holoscan SDK 文檔](https://docs.nvidia.com/holoscan/)
- [OpenCV 文檔](https://docs.opencv.org/)
- [Docker 文檔](https://docs.docker.com/)

## 🤝 貢獻

歡迎提交 Issues 和 Pull Requests！

## 📄 許可證

本項目採用 [MIT 許可證](LICENSE)

---

💡 **提示**：如果您是第一次使用，建議直接運行 `make all` 來體驗完整的一鍵式流程！

## 運行應用程序

### 基本運行
```bash
# 使用 Makefile（推薦）
make run

# 或直接使用腳本
./scripts/run.sh
```

### Headless 模式（無顯示視窗）

**重要**: HolovizOp 的 headless 模式通過 `enable_render_buffer_output=true` 實現，這樣可以在無顯示環境下運行，同時保持完整的渲染管道功能。

```bash
# 使用 Makefile
make run-headless

# 或使用腳本參數
./scripts/run.sh --headless

# 或設置環境變量
export HOLOVIZ_HEADLESS=1
./scripts/run.sh
```

### 其他運行選項
```bash
# 指定數據路徑
./scripts/run.sh --data /path/to/data

# 指定源類型
./scripts/run.sh --source yuan

# 設置日誌級別
./scripts/run.sh --log-level DEBUG

# 組合使用
./scripts/run.sh --headless --log-level INFO --data ./data
``` 