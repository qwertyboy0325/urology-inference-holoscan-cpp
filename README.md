# Urology Inference Holoscan C++

A high-performance C++ implementation of urology inference application using NVIDIA Holoscan SDK for real-time medical image segmentation and analysis.

## Overview

This application performs real-time YOLO-based segmentation on urology medical images, identifying and segmenting various anatomical structures including:

- Spleen
- Left Kidney
- Left Renal Artery
- Left Renal Vein
- Left Ureter
- Psoas Muscle
- And other urological structures

## Features

- **Real-time Processing**: Built with NVIDIA Holoscan SDK for high-performance streaming
- **GPU Acceleration**: TensorRT inference with FP16 optimization
- **Multiple Input Sources**: Support for video files and capture cards
- **Advanced Visualization**: Real-time overlay of segmentation results
- **Configurable Pipeline**: YAML-based configuration system
- **Recording Capability**: Optional video recording with H.264 encoding

## Requirements

### System Requirements
- NVIDIA GPU with CUDA support
- Ubuntu 20.04 or later
- NVIDIA Holoscan SDK 2.1+
- CMake 3.20+
- C++17 compiler

### Dependencies
- NVIDIA Holoscan SDK
- OpenCV 4.x
- YAML-cpp
- CUDA Toolkit
- TensorRT
- Qt6 (optional, for GUI)

## Installation

### 1. Install Holoscan SDK
Follow the [official Holoscan installation guide](https://docs.nvidia.com/holoscan/sdk-user-guide/installation.html).

### 2. Install Dependencies
```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    libopencv-dev \
    libyaml-cpp-dev \
    qt6-base-dev \
    pkg-config
```

### 3. Build the Application
```bash
cd urology-inference-holoscan-cpp
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Usage

### Basic Usage
```bash
./urology_inference_holoscan_cpp -d /path/to/data
```

### Command Line Options
- `-d, --data PATH`: Set the input data directory
- `-s, --source TYPE`: Set source type (`replayer` or `yuan`)
- `-o, --output FILE`: Set output filename for recording
- `-l, --labels FILE`: Set custom labels file
- `-h, --help`: Show help message

### Configuration

The application uses YAML configuration files located in `src/config/`:

- `app_config.yaml`: Main application configuration
- `labels.yaml`: Class labels and colors

#### Example Configuration
```yaml
source: "replayer"
model_name: "Urology_yolov11x-seg_3-13-16-17val640rezize_1_4.40.0_nms.onnx"
yolo_postprocessor:
  scores_threshold: 0.2
  num_class: 12
```

## Directory Structure

```
urology-inference-holoscan-cpp/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── urology_app.hpp
│   ├── operators/
│   │   └── yolo_seg_postprocessor.hpp
│   └── utils/
│       └── yolo_utils.hpp
├── src/
│   ├── main.cpp
│   ├── urology_app.cpp
│   ├── operators/
│   │   ├── yolo_seg_postprocessor.cpp
│   │   ├── dummy_receiver.cpp
│   │   └── passthrough.cpp
│   ├── utils/
│   │   ├── yolo_utils.cpp
│   │   └── cv_utils.cpp
│   └── config/
│       ├── app_config.yaml
│       ├── labels.yaml
│       └── app_config.cpp
├── data/
│   ├── models/
│   ├── inputs/
│   └── output/
└── scripts/
```

## Model Setup

1. Download the YOLO segmentation model:
   - Contact your administrator for the model file
   - Place the model in `data/models/` directory

2. Prepare input data:
   - For video files: Place in `data/inputs/`
   - For live capture: Configure capture card settings

## Pipeline Architecture

The application uses a multi-stage processing pipeline:

1. **Video Source**: Video stream replayer or capture card input
2. **Preprocessing**: Format conversion and resizing (640x640)
3. **Inference**: TensorRT-based YOLO inference
4. **Postprocessing**: YOLO output parsing and NMS
5. **Visualization**: Real-time overlay rendering
6. **Recording** (optional): H.264 video encoding

## Performance Optimization

- **Memory Pools**: Efficient GPU/CPU memory management
- **CUDA Streams**: Asynchronous processing pipeline
- **TensorRT**: FP16 inference optimization
- **Zero-Copy**: Direct GPU memory operations

## Troubleshooting

### Common Issues

1. **Model Loading Error**
   - Verify model file path and permissions
   - Check TensorRT compatibility

2. **CUDA Out of Memory**
   - Reduce batch size or input resolution
   - Adjust memory pool sizes

3. **Performance Issues**
   - Enable FP16 inference
   - Check GPU utilization
   - Verify CUDA streams configuration

### Debugging

Enable debug logging:
```cpp
holoscan::set_log_level(holoscan::LogLevel::DEBUG);
```

## Development

### Adding New Operators
1. Create header file in `include/operators/`
2. Implement operator in `src/operators/`
3. Register in `CMakeLists.txt`
4. Update pipeline in `urology_app.cpp`

### Custom Postprocessing
Extend `YoloSegPostprocessorOp` class for custom processing logic.

## License

Apache License 2.0 - See LICENSE file for details.

## Contributing

1. Fork the repository
2. Create feature branch
3. Make changes with tests
4. Submit pull request

## Support

For issues and questions:
- Check the troubleshooting section
- Review Holoscan SDK documentation
- Contact the development team

## Acknowledgments

- NVIDIA Holoscan SDK team
- Original Python implementation contributors
- Medical imaging domain experts 

## 新增功能 - Video Encoder 支持

此版本已完整實現基於holohub H.264應用程序的video encoder功能，與Python版本功能對等。

### Video Encoder 組件

基於holohub的h264_endoscopy_tool_tracking應用程序，我們實現了以下組件：

#### 1. VideoEncoderRequestOp
- 處理YUV幀到H.264位元流的編碼請求
- 支持硬體加速編碼
- 可配置編碼參數（bitrate, framerate, profile等）

#### 2. VideoEncoderResponseOp  
- 處理編碼後的H.264位元流輸出
- 管理編碼器響應和緩衝區

#### 3. VideoEncoderContext
- 保存編碼器上下文和狀態
- 支持異步調度條件

#### 4. VideoWriteBitstreamOp
- 將H.264位元流寫入磁碟
- 支持CRC檢查和檔案管理

#### 5. TensorToVideoBufferOp
- 將Tensor數據轉換為視頻緩衝區
- 支持YUV420格式轉換

### 錄製Pipeline架構

```
Holoviz (render_buffer_output) 
    ↓
HolovizOutputFormatConverter (RGBA→RGB)
    ↓  
EncoderInputFormatConverter (RGB→YUV420)
    ↓
TensorToVideoBufferOp
    ↓
VideoEncoderRequestOp → VideoEncoderResponseOp
    ↓
VideoWriteBitstreamOp → Output H.264 file
```

### 配置參數

錄製功能可通過以下參數配置：

```yaml
# 基本錄製設置
record_output: true

# 編碼器參數
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

# 格式轉換參數
holoviz_output_format_converter:
  in_dtype: "rgba8888"
  out_dtype: "rgb888"
  resize_width: 1920
  resize_height: 1080

encoder_input_format_converter:
  in_dtype: "rgb888"
  out_dtype: "yuv420"
```

### GXF擴展支持

應用程序會自動加載以下GXF擴展：
- `libgxf_videoencoder.so` - 視頻編碼器核心
- `libgxf_videoencoderio.so` - 視頻編碼器I/O
- `libgxf_videodecoder.so` - 視頻解碼器
- `libgxf_videodecoderio.so` - 視頻解碼器I/O

### 使用方法

1. **啟用錄製功能**：
   ```bash
   ./build/urology_inference_holoscan_cpp -d ./data --record_output true
   ```

2. **自定義輸出檔案**：
   ```bash
   ./build/urology_inference_holoscan_cpp -o my_recording.h264
   ```

3. **運行時控制錄製**：
   ```cpp
   app->toggle_record();        // 切換錄製狀態
   app->set_record_enabled(true); // 啟用錄製
   ```

### 輸出檔案

- **視頻檔案**：`data/output/{timestamp}.h264`
- **CRC檔案**：`data/output/surgical_video_output.txt`
- **日誌檔案**：包含編碼統計和錯誤資訊

### 性能優化

- **GPU加速**：使用CUDA streams進行異步處理
- **內存池**：優化的塊內存分配器
- **硬體編碼**：NVENC H.264硬體編碼器
- **零拷貝**：GPU Direct RDMA支持（未來版本）

### 與Python版本的對比

| 功能 | Python版本 | C++版本 | 狀態 |
|------|------------|---------|------|
| 基本錄製 | ✅ | ✅ | 完成 |
| H.264編碼 | ✅ | ✅ | 完成 |
| 動態錄製控制 | ✅ | ✅ | 完成 |
| 編碼參數配置 | ✅ | ✅ | 完成 |
| PassThrough操作符 | ✅ | 🚧 | 計劃中 |
| YUAN擷取卡支持 | ✅ | 🚧 | 計劃中 |

### 故障排除

1. **GXF擴展加載失敗**：
   - 確保Holoscan SDK正確安裝
   - 檢查LD_LIBRARY_PATH包含GXF擴展路徑

2. **編碼錯誤**：
   - 驗證GPU驅動程序支持NVENC
   - 檢查視頻格式和參數設置

3. **內存不足**：
   - 調整內存池大小
   - 降低視頻解析度或bitrate

### 參考資料

- [NVIDIA Holoscan SDK文檔](https://docs.nvidia.com/holoscan/sdk-user-guide/)
- [HoloHub H.264應用程序](https://github.com/nvidia-holoscan/holohub/tree/main/applications/h264)
- [Video Encoder Operator文檔](https://github.com/nvidia-holoscan/holohub/tree/main/operators/video_encoder) 