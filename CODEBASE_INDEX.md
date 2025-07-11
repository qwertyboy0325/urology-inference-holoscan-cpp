# Urology Inference Holoscan C++ - Codebase Index

## 📋 Project Overview

This is a **medical imaging analysis system** for urology based on NVIDIA Holoscan SDK 3.3.0. The application performs real-time video processing and AI inference for surgical tool detection and segmentation.

### 🎯 Key Features
- **GPU-accelerated inference** using NVIDIA GPUs
- **Real-time video processing** with YOLO segmentation models
- **Containerized deployment** with Docker
- **Headless mode support** for server environments
- **Interactive controls** (pause/resume/record)

## 🏗️ Architecture Overview

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Video Input   │───▶│  Preprocessing  │───▶│   AI Inference  │
│   (Replayer)    │    │  (Format Conv)  │    │   (YOLO Seg)    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                                       │
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Visualization  │◀───│  Postprocessing │◀───│   GPU Memory    │
│   (HolovizOp)   │    │  (YOLO Seg)     │    │   Management    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## 📁 Directory Structure

### Core Application Files
- **`src/main.cpp`** - Application entry point with command-line parsing
- **`src/urology_app.cpp`** - Main application logic and pipeline composition
- **`include/urology_app.hpp`** - Main application class definition

### Operators (Custom Processing Components)
- **`src/operators/yolo_seg_postprocessor.cpp`** - YOLO segmentation post-processing
- **`src/operators/yolo_seg_postprocessor.cu`** - CUDA kernels for GPU acceleration
- **`include/operators/yolo_seg_postprocessor.hpp`** - Post-processor interface

### Utilities
- **`src/utils/yolo_utils.cpp`** - YOLO model utility functions
- **`src/utils/logger.cpp`** - High-performance logging system
- **`src/utils/error_handler.cpp`** - Error handling and recovery
- **`src/utils/performance_monitor.cpp`** - Performance monitoring
- **`src/utils/cv_utils.cpp`** - OpenCV utility functions

### Configuration
- **`src/config/app_config.cpp`** - Application configuration management
- **`src/config/app_config.yaml`** - YAML configuration files
- **`data/config/labels.yaml`** - Surgical tool labels and colors

### Build System
- **`CMakeLists.txt`** - CMake build configuration
- **`Makefile`** - Simplified build commands
- **`Dockerfile`** - Multi-stage container build
- **`docker-compose.yml`** - Container orchestration

## 🔧 Key Components

### 1. Main Application (`UrologyApp`)

**Location**: `include/urology_app.hpp`, `src/urology_app.cpp`

**Purpose**: Orchestrates the entire inference pipeline

**Key Features**:
- Pipeline composition and management
- Memory pool initialization (host/device/CUDA streams)
- Video replayer setup
- Inference operator configuration
- Visualization setup (HolovizOp)
- Recording pipeline (video encoder)

**Core Methods**:
```cpp
class UrologyApp : public holoscan::Application {
    void compose() override;           // Pipeline composition
    void toggle_pipeline();            // Pause/resume control
    void toggle_record();              // Recording control
    bool is_recording() const;         // Recording status
};
```

### 2. YOLO Segmentation Post-Processor

**Location**: `include/operators/yolo_seg_postprocessor.hpp`, `src/operators/yolo_seg_postprocessor.cpp`

**Purpose**: Processes YOLO model outputs for segmentation and detection

**Key Features**:
- GPU-accelerated post-processing with CUDA kernels
- Non-Maximum Suppression (NMS)
- Box coordinate conversion (xywh ↔ xyxy)
- Confidence threshold filtering
- Segmentation mask processing

**CUDA Kernels** (`yolo_seg_postprocessor.cu`):
- `yolo_postprocess_kernel` - Main post-processing kernel
- `nms_kernel` - Non-Maximum Suppression kernel

### 3. Video Processing Pipeline

**Components**:
1. **VideoStreamReplayerOp** - Reads video data
2. **FormatConverterOp** - Preprocessing (resize, format conversion)
3. **InferenceOp** - YOLO model inference
4. **YoloSegPostprocessorOp** - Post-processing
5. **HolovizOp** - Visualization

### 4. Memory Management

**Memory Pools**:
- `host_memory_pool_` - CPU memory (256MB blocks)
- `device_memory_pool_` - GPU memory (256MB blocks)
- `cuda_stream_pool_` - CUDA stream management

### 5. Logging System

**Location**: `include/utils/logger.hpp`, `src/utils/logger.cpp`

**Features**:
- Thread-safe logging
- Multiple output targets (console/file)
- Performance timing macros
- Log level filtering
- RAII performance timers

**Usage**:
```cpp
UROLOGY_LOG_INFO("Processing frame " << frame_id);
UROLOGY_PERF_TIMER("inference_time");
```

## 🚀 Build and Deployment

### Build System

**CMake Configuration** (`CMakeLists.txt`):
- C++17 standard
- CUDA support enabled
- Holoscan SDK integration
- OpenCV integration
- Multiple executables (main, test, minimal)

**Makefile Commands**:
```bash
make build    # Build application
make run      # Run application
make test     # Run tests
make dev      # Start development environment
make clean    # Clean build files
```

### Containerization

**Docker Stages**:
1. **Builder** - Compilation and testing
2. **Runtime** - Minimal runtime environment
3. **Development** - Full development environment

**Docker Compose Services**:
- `urology-dev` - Development environment
- `urology-run` - Runtime execution
- `urology-test` - Testing environment

## 🎮 Runtime Controls

### Interactive Commands
- **P** - Pause/Resume pipeline
- **R** - Toggle recording
- **Q** - Quit application

### Command-Line Options
```bash
./urology_inference_holoscan_cpp \
    --data /path/to/data \
    --source replayer \
    --output output.mp4 \
    --labels labels.yaml
```

### Environment Variables
- `HOLOHUB_DATA_PATH` - Data directory
- `HOLOSCAN_MODEL_PATH` - Model directory
- `HOLOVIZ_HEADLESS` - Headless mode
- `LOG_LEVEL` - Logging level

## 🔬 AI Model Integration

### Model Details
- **Framework**: ONNX Runtime
- **Model**: YOLOv9 with segmentation
- **Input**: RGB888, 640x640
- **Output**: Detection boxes + segmentation masks
- **Classes**: 12 surgical tool classes

### Model Classes
```cpp
// Surgical tool labels
{0, "Background"}
{1, "Spleen"}
{2, "Left_Kidney"}
{3, "Left_Renal_Artery"}
{4, "Left_Renal_Vein"}
{5, "Left_Ureter"}
// ... more classes
```

### GPU Acceleration
- **CUDA kernels** for post-processing
- **TensorRT** for inference optimization
- **Memory pools** for efficient GPU memory management

## 🛠️ Development Tools

### Testing
- **Unit tests** in `tests/unit/`
- **Performance benchmarks** in `tests/performance/`
- **Integration tests** via Docker

### Debugging
- **GDB** integration
- **Valgrind** memory checking
- **Clang-tidy** static analysis
- **Cppcheck** code analysis

### Performance Monitoring
- **Performance timers** with RAII
- **Memory usage tracking**
- **GPU utilization monitoring**
- **Pipeline latency measurement**

## 📊 Data Flow

### Input Processing
1. **Video Replayer** reads GXF format video
2. **Format Converter** resizes to 640x640
3. **Preprocessing** converts to float32

### Inference Pipeline
1. **InferenceOp** runs YOLO model
2. **Post-processor** processes outputs
3. **NMS** removes duplicate detections
4. **Visualization** renders results

### Output Generation
1. **HolovizOp** displays visualization
2. **Video Encoder** records output (optional)
3. **File Writer** saves results

## 🔧 Configuration

### Application Config (`app_config.yaml`)
```yaml
model:
  path: "models/urology_yolov9c_3000random640resize_20240811_4.34_nhwc.NVIDIAL4.8.9.58.trt.10.3.0.26.engine.fp16"
  type: "onnx"
  input_size: [640, 640]

pipeline:
  frame_rate: 30
  enable_recording: true
  headless_mode: false
```

### Labels Config (`labels.yaml`)
```yaml
labels:
  - ["Background", 0.0, 0.0, 0.0, 0.0]
  - ["Spleen", 0.1451, 0.9412, 0.6157, 0.2]
  # ... more labels
```

## 🚨 Error Handling

### Error Categories
- **Model loading errors** - Missing model files
- **GPU errors** - CUDA kernel failures
- **Memory errors** - Pool allocation failures
- **Pipeline errors** - Operator failures

### Recovery Mechanisms
- **Graceful degradation** - Fallback to CPU
- **Error logging** - Detailed error reporting
- **Resource cleanup** - RAII patterns
- **Health checks** - System monitoring

## 📈 Performance Optimization

### GPU Optimizations
- **CUDA streams** for parallel processing
- **Memory pools** for efficient allocation
- **Kernel optimization** for post-processing
- **TensorRT** for inference acceleration

### Pipeline Optimizations
- **Zero-copy** data transfer where possible
- **Batch processing** for multiple frames
- **Async processing** for I/O operations
- **Memory pinning** for GPU transfers

## 🔍 Troubleshooting

### Common Issues
1. **GPU not detected** - Check NVIDIA drivers
2. **Model not found** - Verify model path
3. **Memory errors** - Increase pool sizes
4. **Display issues** - Check X11 forwarding

### Debug Commands
```bash
# Check GPU status
nvidia-smi

# Verify model file
ls -la data/models/

# Check container logs
docker logs urology-dev

# Run with debug logging
LOG_LEVEL=DEBUG ./urology_inference_holoscan_cpp
```

## 📚 API Reference

### Main Application API
```cpp
// Create application
auto app = std::make_shared<urology::UrologyApp>(
    data_path, source, output_filename, labels_file);

// Run application
app->run();

// Control pipeline
app->toggle_pipeline();
app->toggle_record();
```

### Post-Processor API
```cpp
// Create post-processor
auto postprocessor = std::make_shared<urology::YoloSegPostprocessorOp>();

// Configure parameters
postprocessor->scores_threshold_ = 0.25;
postprocessor->num_class_ = 12;
```

### Logger API
```cpp
// Initialize logger
auto& logger = urology::utils::UrologyLogger::getInstance();
logger.initialize("app.log", urology::utils::UrologyLogLevel::INFO);

// Log messages
UROLOGY_LOG_INFO("Processing frame " << frame_id);
UROLOGY_LOG_ERROR("GPU error: " << error_msg);
```

## 🎯 Future Enhancements

### Planned Features
- **Multi-model support** - Multiple AI models
- **Web interface** - REST API + Web UI
- **Real-time streaming** - Live video input
- **Advanced analytics** - Performance metrics
- **Plugin system** - Extensible architecture

### Performance Improvements
- **Multi-GPU support** - Distributed processing
- **Model quantization** - INT8 inference
- **Pipeline optimization** - Reduced latency
- **Memory optimization** - Reduced footprint

---

This index provides a comprehensive overview of the urology inference holoscan C++ codebase. For detailed implementation, refer to the individual source files and their documentation. 