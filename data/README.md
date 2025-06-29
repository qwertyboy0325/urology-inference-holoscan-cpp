# Data Directory

This directory contains the data files required for the Urology Inference application.

## Directory Structure

```
data/
├── models/          # AI models (ONNX, TensorRT engines)
├── inputs/          # Input video files for replayer mode
└── output/          # Generated output files and recordings
```

## Models

Place your YOLO segmentation model files in the `models/` directory:

- **Required**: `Urology_yolov11x-seg_3-13-16-17val640rezize_1_4.40.0_nms.onnx`
- **Optional**: Pre-converted TensorRT engine files for faster loading

### Model Format

The application supports:
- ONNX models (`.onnx`)
- TensorRT engines (`.engine`, `.plan`)

TensorRT engines are automatically generated from ONNX models during first run.

## Input Data

For replayer mode, place your input video files in the `inputs/` directory:

- Supported formats: `.mp4`, `.avi`, `.mov`, `.gxf_entities`
- Recommended resolution: 1920x1080
- Frame rate: 30 FPS

### GXF Entities Format

The application can also use Holoscan's native GXF entities format:
- `.gxf_entities` - Serialized tensor data
- `.gxf_index` - Frame index and timestamp information

## Output

The `output/` directory will contain:
- Recorded videos (if recording is enabled)
- Processed results
- Inference logs and statistics

## Getting Started

1. **Download the Model**:
   - Contact your system administrator for the model file
   - Place it in `data/models/`

2. **Prepare Input Data**:
   - For video files: Copy to `data/inputs/`
   - For live capture: No additional setup needed

3. **Run the Application**:
   ```bash
   ./scripts/run.sh -d ./data
   ```

## Environment Variables

- `HOLOHUB_DATA_PATH`: Override default data directory path
- `HOLOSCAN_MODEL_PATH`: Override default model directory path

## Notes

- Model files are large (>100MB) and should not be committed to git
- Input videos may contain sensitive medical data - handle appropriately
- Output recordings are automatically timestamped 