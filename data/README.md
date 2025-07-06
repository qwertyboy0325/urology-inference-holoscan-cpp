# Urology Inference Data Directory

This directory contains all data files required for the Urology Inference application.

## 📁 Directory Structure

```
data/
├── models/          # AI models (ONNX, TensorRT engines)
├── inputs/          # Input videos in GXF format
├── config/          # Configuration files
├── output/          # Generated output videos and results
├── logs/            # Application logs
└── videos/          # Source videos for conversion (optional)
```

## 🤖 AI Models

### Required Model
Place your YOLO segmentation model in the `models/` directory:
- **Required**: `Urology_yolov11x-seg_3-13-16-17val640rezize_1_4.40.0_nms.onnx`
- **Optional**: Pre-converted TensorRT engine files (`.engine`, `.plan`)

### Model Setup
```bash
# Download model (contact system administrator)
# Place in data/models/
cp your_model.onnx data/models/

# Verify model
ls -lh data/models/Urology_yolov11x-seg_3-13-16-17val640rezize_1_4.40.0_nms.onnx
```

## 🎥 Video Input Data

### Important: GXF Format Requirement
**The VideoStreamReplayerOp only supports GXF format**, not standard video files (MP4, AVI, etc.).

### Supported Input Formats
- ✅ `.gxf_entities` - Serialized tensor data
- ✅ `.gxf_index` - Frame index and timestamp information
- ❌ `.mp4`, `.avi`, `.mov` - **NOT supported directly**

## 🔄 Video Format Conversion

### Converting Standard Videos to GXF Format

#### Method 1: Using Holoscan Conversion Script
```bash
# 1. Install dependencies
pip3 install opencv-python numpy

# 2. Clone Holoscan SDK for conversion script
git clone https://github.com/nvidia-holoscan/holoscan-sdk.git
cd holoscan-sdk

# 3. Convert your video
ffmpeg -i your_video.mp4 -pix_fmt rgb24 -f rawvideo pipe:1 | \
python3 scripts/convert_video_to_gxf_entities.py \
    --width 1920 \
    --height 1080 \
    --channels 3 \
    --directory ../data/inputs \
    --basename tensor

# This creates:
# - data/inputs/tensor.gxf_entities
# - data/inputs/tensor.gxf_index
```

#### Method 2: Using Docker Container
```bash
# Run conversion inside Holoscan container
docker run -it --rm \
    -v $(pwd)/data:/workspace/data \
    -v $(pwd)/videos:/workspace/videos \
    nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu bash

# Inside container:
cd /workspace
ffmpeg -i videos/your_video.mp4 -pix_fmt rgb24 -f rawvideo pipe:1 | \
python3 /opt/nvidia/holoscan/examples/scripts/convert_video_to_gxf_entities.py \
    --width 1920 --height 1080 --channels 3 \
    --directory data/inputs --basename tensor
```

#### Method 3: Batch Conversion Script
```bash
#!/bin/bash
# save as: scripts/convert_videos.sh

INPUT_DIR="data/videos"
OUTPUT_DIR="data/inputs"
HOLOSCAN_SCRIPT="/opt/nvidia/holoscan/examples/scripts/convert_video_to_gxf_entities.py"

mkdir -p "$OUTPUT_DIR"

for video in "$INPUT_DIR"/*.{mp4,avi,mov}; do
    if [ -f "$video" ]; then
        basename=$(basename "$video" | sed 's/\.[^.]*$//')
        echo "Converting: $video -> $basename"
        
        ffmpeg -i "$video" -pix_fmt rgb24 -f rawvideo pipe:1 | \
        python3 "$HOLOSCAN_SCRIPT" \
            --width 1920 --height 1080 --channels 3 \
            --directory "$OUTPUT_DIR" --basename "$basename"
    fi
done
```

### Video Conversion Parameters
| Parameter | Description | Recommended Value |
|-----------|-------------|-------------------|
| `--width` | Video width | 1920 |
| `--height` | Video height | 1080 |
| `--channels` | Color channels | 3 (RGB) |
| `--basename` | Output filename prefix | tensor |
| `--directory` | Output directory | data/inputs |

## 🐳 Docker Usage

### Optimized Volume Mounting
```bash
# Simple Docker run with organized mounts
docker run -it --rm --gpus all \
    -v $(pwd)/data/models:/app/models:ro \
    -v $(pwd)/data/inputs:/app/inputs:ro \
    -v $(pwd)/data/config:/app/config:ro \
    -v $(pwd)/data/output:/app/output \
    -v $(pwd)/data/logs:/app/logs \
    urology-inference:runtime

# Or use docker-compose (recommended)
docker-compose up urology-inference
```

### Directory Mapping
| Host Directory | Container Path | Purpose | Access |
|----------------|----------------|---------|---------|
| `./data/models` | `/app/models` | AI models | Read-only |
| `./data/inputs` | `/app/inputs` | Input videos (GXF) | Read-only |
| `./data/config` | `/app/config` | Configuration files | Read-only |
| `./data/output` | `/app/output` | Generated outputs | Read-write |
| `./data/logs` | `/app/logs` | Application logs | Read-write |

## 📋 Configuration Files

### Required Configuration Files
Place in `data/config/`:
- `app_config.yaml` - Main application configuration
- `labels.yaml` - YOLO class labels

### Sample Configuration
```yaml
# data/config/app_config.yaml
source: "replayer"
model_name: "Urology_yolov11x-seg_3-13-16-17val640rezize_1_4.40.0_nms.onnx"

replayer:
  basename: "tensor"  # Must match your GXF files
  frame_rate: 30
  repeat: false
  realtime: false
```

## 📊 Output Files

The `output/` directory will contain:
- **Processed videos**: H.264 encoded results with segmentation overlays
- **Inference logs**: Performance metrics and statistics
- **Debug outputs**: Intermediate processing results (if enabled)

## 🚀 Quick Start Guide

### 1. Prepare Your Data
```bash
# Create directory structure
mkdir -p data/{models,inputs,config,output,logs,videos}

# Place your model
cp your_model.onnx data/models/

# Place your source videos
cp your_video.mp4 data/videos/
```

### 2. Convert Videos to GXF Format
```bash
# Using Docker for conversion
docker run -it --rm \
    -v $(pwd)/data:/workspace/data \
    nvcr.io/nvidia/clara-holoscan/holoscan:v3.3.0-dgpu bash -c "
    ffmpeg -i /workspace/data/videos/your_video.mp4 -pix_fmt rgb24 -f rawvideo pipe:1 | \
    python3 /opt/nvidia/holoscan/examples/scripts/convert_video_to_gxf_entities.py \
        --width 1920 --height 1080 --channels 3 \
        --directory /workspace/data/inputs --basename tensor
"
```

### 3. Run the Application
```bash
# Using docker-compose
docker-compose up urology-inference

# Or direct Docker run
docker run -it --rm --gpus all \
    -v $(pwd)/data/models:/app/models:ro \
    -v $(pwd)/data/inputs:/app/inputs:ro \
    -v $(pwd)/data/config:/app/config:ro \
    -v $(pwd)/data/output:/app/output \
    -v $(pwd)/data/logs:/app/logs \
    urology-inference:runtime
```

## 🔧 Troubleshooting

### Common Issues

#### 1. "No input data found"
```bash
# Check GXF files exist
ls -la data/inputs/
# Should show: tensor.gxf_entities, tensor.gxf_index

# Verify file sizes
ls -lh data/inputs/tensor.*
```

#### 2. "Model file not found"
```bash
# Check model file
ls -la data/models/
# Should show your .onnx model file

# Verify model name in config
grep model_name data/config/app_config.yaml
```

#### 3. Video Conversion Errors
```bash
# Check video properties
ffprobe your_video.mp4

# Test conversion manually
ffmpeg -i your_video.mp4 -pix_fmt rgb24 -f rawvideo -t 1 test_output.raw
```

## 📝 Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `HOLOSCAN_MODEL_PATH` | Model directory path | `/app/models` |
| `HOLOSCAN_INPUT_PATH` | Input directory path | `/app/inputs` |
| `UROLOGY_CONFIG_PATH` | Configuration directory | `/app/config` |
| `UROLOGY_OUTPUT_PATH` | Output directory | `/app/output` |
| `UROLOGY_LOG_PATH` | Log directory | `/app/logs` |

## 🔒 Security Notes

- Model files may be large (>100MB) - use `.gitignore`
- Input videos may contain sensitive medical data - handle appropriately
- Use read-only mounts for models, inputs, and config
- Logs and outputs use read-write mounts for data persistence

## 💡 Performance Tips

1. **Use SSD storage** for input/output directories
2. **Pre-convert videos** to GXF format for faster loading
3. **Use TensorRT engines** instead of ONNX for better performance
4. **Monitor GPU memory** usage with `nvidia-smi`
5. **Adjust batch sizes** based on available GPU memory 