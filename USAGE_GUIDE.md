# 🚀 Urology Inference Holoscan C++ - 使用指南

## ✨ 系統特點

- 🎯 **預裝依賴**：所有依賴都在 Dockerfile 中預裝，無需每次啟動時安裝
- 🐳 **完全容器化**：不會影響主機系統環境
- ⚡ **快速啟動**：無需等待依賴安裝，直接開始構建
- 🖥️ **X11 GUI 支持**：支持圖形界面顯示
- 🔥 **GPU 加速**：支持 NVIDIA GPU 加速推理

## 📦 預裝的依賴

- ✅ **NVIDIA Holoscan SDK 3.3.0**
- ✅ **OpenCV 4.6.0**（完整模組）
- ✅ **FFmpeg**（完整視頻處理支持）
- ✅ **X11 GUI 庫**（GTK3、X11 等）
- ✅ **開發工具**（CMake、Ninja、GDB 等）

## 🚀 快速開始

### 1. X11 設置（在主機上執行）

```bash
# 允許 Docker 容器連接到 X11 顯示
xhost +local:docker

# 設置 DISPLAY 環境變量
export DISPLAY=:0
```

### 2. 一鍵式使用

```bash
# 完整流程：設置 + 構建 + 測試
make all

# 運行應用程序（帶 X11 GUI）
make run
```

## 📋 可用命令

| 命令 | 說明 | 特點 |
|------|------|------|
| `make all` | 完整流程：設置 + 構建 + 測試 | 🎯 一鍵完成所有步驟 |
| `make build` | 構建應用程序 | ⚡ 使用預裝依賴，快速構建 |
| `make run` | 運行應用程序 | 🖥️ 支持 X11 GUI 顯示 |
| `make test` | 運行快速測試 | 🧪 驗證系統狀態 |
| `make dev` | 啟動開發環境 | 💻 進入交互式開發模式 |
| `make clean` | 清理構建文件 | 🧹 清理舊的構建產物 |
| `make info` | 查看項目信息 | 📊 顯示構建狀態和配置 |

## 🔄 使用流程

### 首次使用

```bash
# 1. 克隆項目
git clone <repository-url>
cd urology-inference-holoscan-cpp

# 2. 設置 X11（如果需要 GUI）
xhost +local:docker
export DISPLAY=:0

# 3. 一鍵完成所有設置
make all
```

### 日常使用

```bash
# 運行應用程序
make run

# 或者查看幫助
make help
```

### 開發模式

```bash
# 啟動開發環境
make dev

# 在容器內進行開發
# 修改代碼後重新構建
make build
```

## 🎥 視頻處理

### 視頻格式轉換

```bash
# 將標準視頻轉換為 GXF 格式
# 1. 將原始視頻放在 data/videos/ 目錄
# 2. 運行轉換腳本
./scripts/convert_videos.sh
```

### 支持的視頻格式

- **輸入**：MP4、AVI、MOV 等標準格式
- **輸出**：GXF 格式（Holoscan 專用）

## 🔧 故障排除

### X11 相關問題

```bash
# X11 連接被拒絕
xhost +local:docker
xhost +local:root

# 檢查 DISPLAY 設置
echo $DISPLAY

# 如果使用 SSH
ssh -X username@hostname
```

### 構建問題

```bash
# 清理並重新構建
make clean
make build

# 查看詳細錯誤
make build 2>&1 | tee build.log
```

### 權限問題

```bash
# 修復文件權限（如果需要）
sudo chown -R $USER:$USER .
```

## 📊 性能優化

### GPU 支持

```bash
# 檢查 GPU 可用性
docker run --rm --gpus all nvidia/cuda:11.0-base nvidia-smi

# 確保 NVIDIA Container Toolkit 已安裝
```

### 構建優化

- ✅ **ccache**：已預配置，加速重複構建
- ✅ **並行編譯**：自動使用所有 CPU 核心
- ✅ **預裝依賴**：避免重複下載和安裝

## 🎯 優勢總結

### 相比之前的系統

| 方面 | 之前 | 現在 |
|------|------|------|
| **依賴安裝** | 每次啟動時安裝 | Dockerfile 預裝 |
| **啟動時間** | 2-3 分鐘 | 10-20 秒 |
| **網路使用** | 每次下載 | 一次構建，多次使用 |
| **環境一致性** | 可能不一致 | 完全一致 |
| **系統影響** | 可能影響主機 | 完全隔離 |

### 技術亮點

- 🏗️ **多階段 Dockerfile**：分離構建和運行環境
- 🔄 **智能緩存**：Docker 層緩存 + ccache
- 🎯 **目標特定**：development/runtime/builder 不同階段
- 🔒 **安全性**：容器化隔離，不影響主機

## 📚 更多信息

- [NVIDIA Holoscan SDK 文檔](https://docs.nvidia.com/holoscan/)
- [OpenCV 文檔](https://docs.opencv.org/)
- [Docker 文檔](https://docs.docker.com/)

---

💡 **提示**：現在您可以直接運行 `make all` 來體驗完整的預裝依賴系統！ 