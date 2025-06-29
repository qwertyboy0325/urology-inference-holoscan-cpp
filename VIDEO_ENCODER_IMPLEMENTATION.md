# Video Encoder 實現總結

## 問題描述

用戶指出 `urology_inference_holoscan.py` 中引用的encoder沒有正確實現在 C++ 版本中，要求根據 holohub 的 application 找出相似項目並修復。

## 解決方案

基於 holohub 文檔中的 H.264 應用程序（特別是 `h264_endoscopy_tool_tracking`），完整實現了 video encoder 功能。

## 實現的組件

### 1. Video Encoder Operators

在 `src/operators/video_encoder_ops.cpp` 中實現了以下 GXF-based operators：

#### VideoEncoderRequestOp
- **功能**: 處理 YUV 幀到 H.264 位元流的編碼請求
- **GXF類型**: `nvidia::gxf::VideoEncoderRequest`
- **參數**: 支持 bitrate, framerate, profile, codec 等配置

#### VideoEncoderResponseOp
- **功能**: 處理編碼器的響應和輸出
- **GXF類型**: `nvidia::gxf::VideoEncoderResponse`
- **特性**: 管理編碼緩衝區和異步處理

#### VideoEncoderContext
- **功能**: 管理編碼器上下文和狀態
- **GXF類型**: `nvidia::gxf::VideoEncoderContext`
- **特性**: 支持異步調度條件

#### VideoWriteBitstreamOp
- **功能**: 將 H.264 位元流寫入磁碟
- **GXF類型**: `nvidia::gxf::VideoWriteBitstream`
- **特性**: 支持 CRC 檢查和檔案管理

#### TensorToVideoBufferOp
- **功能**: 將 Tensor 數據轉換為視頻緩衝區
- **特性**: 支持 YUV420 格式轉換

### 2. Recording Pipeline 架構

實現了完整的錄製 pipeline：

```
Source Video → HoloViz (render_buffer_output)
                ↓
        HolovizOutputFormatConverter (RGBA→RGB)
                ↓
        EncoderInputFormatConverter (RGB→YUV420)
                ↓
            TensorToVideoBufferOp
                ↓
    VideoEncoderRequestOp → VideoEncoderResponseOp
                ↓
            VideoWriteBitstreamOp → H.264 Output File
```

### 3. 應用程序整合

#### 主應用更新 (`src/urology_app.cpp`)
- 更新 `setup_recording_pipeline()` 方法實現完整的錄製功能
- 添加 `load_gxf_extensions()` 方法自動加載所需的 GXF 擴展
- 更新 HoloViz 配置支持 render buffer output

#### 頭文件更新 (`include/urology_app.hpp`)
- 添加所有 video encoder operators 的聲明
- 增加錄製相關的成員變數

### 4. 構建系統更新

#### CMakeLists.txt
- 添加 `src/operators/video_encoder_ops.cpp` 到構建列表

#### 構建腳本 (`scripts/build.sh`)
- 添加 GXF video encoder 擴展的檢查
- 提供友好的警告信息和安裝建議

### 5. 依賴安裝支持

#### 安裝腳本 (`scripts/install_video_encoder_deps.sh`)
- 自動下載和安裝所需的 GXF multimedia 擴展
- 支持 x86_64 和 aarch64 架構
- 包含 DeepStream 依賴支持

## 配置支持

### YAML 配置文件
保持與 Python 版本相同的配置格式：

```yaml
record_output: true

video_encoder_request:
  inbuf_storage_type: 1
  codec: 0                    # H.264
  input_width: 1920
  input_height: 1080
  input_format: "yuv420planar"
  profile: 2                  # High Profile
  bitrate: 20000000          # 20 Mbps
  framerate: 30
  config: "pframe_cqp"
  rate_control_mode: 0
  qp: 20
  iframe_interval: 5
```

## 與 Python 版本的對比

| 功能 | Python 版本 | C++ 版本 | 狀態 |
|------|------------|---------|------|
| 基本錄製 | ✅ | ✅ | 完成 |
| H.264 編碼 | ✅ | ✅ | 完成 |
| 動態錄製控制 | ✅ | ✅ | 完成 |
| 編碼參數配置 | ✅ | ✅ | 完成 |
| GXF 擴展加載 | ✅ | ✅ | 完成 |
| Pipeline 架構 | ✅ | ✅ | 完成 |

## 技術特點

### 1. 硬體加速支持
- 使用 NVIDIA NVENC 硬體編碼器
- 支持 GPU Direct RDMA（與 holohub 應用一致）
- CUDA streams 異步處理

### 2. 內存管理
- 使用 Holoscan BlockMemoryPool 進行優化分配
- 支持 device/host 內存類型選擇
- 零拷貝設計（在支持的操作中）

### 3. 錯誤處理
- 完整的異常處理機制
- 友好的錯誤信息和建議
- 優雅的降級處理（錄製失敗時不影響主功能）

## 參考資料

實現基於以下 holohub 資源：

1. **h264_endoscopy_tool_tracking**: 主要參考應用
2. **video_encoder operator**: 核心編碼器實現
3. **h264_video_decode**: H.264 處理示例
4. **GXF multimedia extensions**: 底層 GXF 組件

## 安裝和使用

### 1. 安裝依賴
```bash
# 安裝 GXF video encoder 擴展
./scripts/install_video_encoder_deps.sh

# 構建應用程序
./scripts/build.sh
```

### 2. 運行應用
```bash
# 啟用錄製功能
./build/urology_inference_holoscan_cpp -d ./data --record_output true

# 自定義輸出檔案
./build/urology_inference_holoscan_cpp -o my_recording.h264
```

### 3. 驗證安裝
```bash
# 檢查 GXF 擴展
ls -la /opt/nvidia/holoscan/lib/ | grep gxf_video
```

## 結論

成功實現了與 Python 版本功能對等的 video encoder 支持，基於 holohub 的最佳實踐和 H.264 應用程序架構。所有組件都遵循 Holoscan SDK 的設計模式，確保了性能和可維護性。 