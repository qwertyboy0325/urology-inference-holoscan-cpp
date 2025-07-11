# YOLO Segmentation Postprocessor - Input/Output Documentation

## 📋 Overview

The `YoloSegPostprocesserOp` is a critical component in the urology inference pipeline that processes raw YOLO model outputs and converts them into structured detection results suitable for visualization and further processing.

## 🏗️ Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Raw YOLO      │───▶│  Postprocessor  │───▶│  Structured     │
│   Model Output  │    │  (GPU/CPU)      │    │  Detections     │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
   ┌─────────────┐      ┌─────────────┐      ┌─────────────┐
   │ Predictions │      │ NMS +       │      │ Boxes +     │
   │ Masks       │      │ Filtering   │      │ Scores +    │
   │             │      │             │      │ Labels      │
   └─────────────┘      └─────────────┘      └─────────────┘
```

## 📥 Inputs

### 1. Primary Input Message (`"in"`)

The operator receives a single input message containing two main tensors:

#### **Predictions Tensor** (`"input_0"`)
- **Type**: `cupy.ndarray` (Python) / `holoscan::Tensor` (C++)
- **Shape**: `[batch_size, num_detections, feature_size]`
- **Format**: `[1, 8400, 38]` (typical YOLO output)
- **Data Type**: `float32`
- **Content**: Raw YOLO model predictions

**Tensor Structure**:
```
[batch][detection][features]
├── [0] x_center      # Normalized center x coordinate
├── [1] y_center      # Normalized center y coordinate  
├── [2] width         # Normalized width
├── [3] height        # Normalized height
├── [4] object_conf   # Object confidence score
├── [5] class_0_conf  # Class 0 confidence
├── [6] class_1_conf  # Class 1 confidence
├── ...
├── [16] class_11_conf # Class 11 confidence
├── [17] mask_0       # Mask coefficient 0
├── [18] mask_1       # Mask coefficient 1
├── ...
└── [37] mask_31      # Mask coefficient 31
```

#### **Segmentation Masks Tensor** (`"input_1"`)
- **Type**: `cupy.ndarray` (Python) / `holoscan::Tensor` (C++)
- **Shape**: `[batch_size, num_prototypes, mask_height, mask_width]`
- **Format**: `[1, 32, 160, 160]` (typical)
- **Data Type**: `float32`
- **Content**: Prototype masks for segmentation

### 2. Configuration Parameters

#### **scores_threshold**
- **Type**: `float`
- **Default**: `0.2`
- **Purpose**: Minimum confidence threshold for detections
- **Range**: `[0.0, 1.0]`

#### **num_class**
- **Type**: `int`
- **Default**: `12`
- **Purpose**: Number of surgical tool classes
- **Values**: `{0: Background, 1: Spleen, 2: Left_Kidney, ...}`

#### **out_tensor_name**
- **Type**: `string`
- **Purpose**: Base name for output tensors
- **Usage**: Used for tensor naming convention

#### **label_dict**
- **Type**: `dict[int, dict]`
- **Purpose**: Class label definitions with colors
- **Structure**:
```python
{
    0: {"text": "Background", "color": [0.0, 0.0, 0.0, 0.0]},
    1: {"text": "Spleen", "color": [0.1451, 0.9412, 0.6157, 0.2]},
    2: {"text": "Left_Kidney", "color": [0.8941, 0.1176, 0.0941, 0.2]},
    # ... more classes
}
```

## 🔄 Processing Pipeline

### 1. Input Reception
```python
def receive_inputs(op_input):
    in_message = op_input.receive("in")
    predictions_tensor = in_message.get("input_0")
    masks_seg_tensor = in_message.get("input_1")
    return cp.asarray(predictions_tensor), cp.asarray(masks_seg_tensor)
```

### 2. Box Processing
```python
output = process_boxes(predictions, masks_seg, self.scores_threshold)
```

**Processing Steps**:
1. **Extract detections** from raw predictions
2. **Apply confidence threshold** filtering
3. **Convert coordinates** from normalized to pixel space
4. **Apply Non-Maximum Suppression (NMS)**
5. **Sort by class IDs** for organized output

### 3. Output Organization
```python
output.sort_by_class_ids()
boxes_cp = cp.reshape(cp.asarray(boxes_cp), (-1, 2, 2))
out_boxes = organize_boxes(class_ids, boxes_cp)
```

## 📤 Outputs

### 1. Main Output Message (`"out"`)

The operator emits a structured output message containing multiple tensors:

#### **Detection Boxes** (`"boxes{class_id}"`)
- **Type**: `cupy.ndarray` (Python) / `holoscan::Tensor` (C++)
- **Shape**: `[1, num_detections, 2, 2]`
- **Format**: `[1, N, 2, 2]` where N = number of detections for this class
- **Content**: Bounding box coordinates in `[x1, y1, x2, y2]` format
- **Example**: `"boxes0"`, `"boxes1"`, `"boxes2"`, etc.

#### **Label Positions** (`"label{class_id}"`)
- **Type**: `cupy.ndarray` (Python) / `holoscan::Tensor` (C++)
- **Shape**: `[1, num_detections, 3]`
- **Format**: `[1, N, 3]` where last dimension is `[x, y, size]`
- **Content**: Text label positions with size information
- **Example**: `"label0"`, `"label1"`, `"label2"`, etc.

#### **Score Positions** (`"score{i}"`)
- **Type**: `cupy.ndarray` (Python) / `holoscan::Tensor` (C++)
- **Shape**: `[1, 1, 3]`
- **Format**: `[1, 1, 3]` where last dimension is `[x, y, size]`
- **Content**: Confidence score display positions
- **Example**: `"score0"`, `"score1"`, `"score2"`, etc.

#### **Segmentation Masks** (`"masks"`)
- **Type**: `cupy.ndarray` (Python) / `holoscan::Tensor` (C++)
- **Shape**: `[num_detections, mask_height, mask_width]`
- **Format**: `[N, 160, 160]` (typical)
- **Content**: Binary segmentation masks for each detection

### 2. Visualization Specs (`"output_specs"`)

The operator also emits visualization specifications for the HolovizOp:

#### **HolovizOp.InputSpec Objects**
```python
spec = HolovizOp.InputSpec(f"score{i}", "text")
spec.color = label["color"]
spec.text = [formated_score]
```

**Properties**:
- **name**: `"score{i}"` where i is detection index
- **type**: `"text"`
- **color**: Class-specific color from label_dict
- **text**: Formatted confidence score (e.g., "0.856")

## 📊 Data Flow Examples

### Example 1: Single Detection
```python
# Input
predictions = [0.5, 0.3, 0.2, 0.1, 0.9, 0.8, 0.1, ...]  # 38 features
masks = [...]  # 32x160x160 prototype masks

# Processing
output = process_boxes(predictions, masks, 0.2)
# Result: 1 detection for class 1 (Spleen)

# Output
out_message = {
    "boxes1": [[[100, 150], [200, 250]]],  # 1 detection box
    "label1": [[[100, 150, 0.04]]],        # Label position
    "score0": [[[200, 150, 0.04]]],        # Score position
    "masks": [...]                          # Segmentation mask
}
```

### Example 2: Multiple Detections
```python
# Input
predictions = [...]  # Multiple detections
masks = [...]        # Multiple masks

# Processing
output = process_boxes(predictions, masks, 0.2)
# Result: 3 detections for class 2 (Left_Kidney)

# Output
out_message = {
    "boxes2": [
        [[100, 150], [200, 250]],  # Detection 1
        [[300, 350], [400, 450]],  # Detection 2
        [[500, 550], [600, 650]]   # Detection 3
    ],
    "label2": [
        [[100, 150, 0.04]],        # Label 1
        [[300, 350, 0.04]],        # Label 2
        [[500, 550, 0.04]]         # Label 3
    ],
    "score0": [[[200, 150, 0.04]]],  # Score 1
    "score1": [[[400, 350, 0.04]]],  # Score 2
    "score2": [[[600, 550, 0.04]]],  # Score 3
    "masks": [...]                    # 3 masks
}
```

## 🔧 Error Handling

### Input Validation
```python
try:
    predictions, masks_seg = self.receive_inputs(op_input)
    output = process_boxes(predictions, masks_seg, self.scores_threshold)
except InferPostProcError as error:
    logger.error(f"Inference Postprocess Error: {error}")
    raise error
```

### Common Error Scenarios
1. **Missing tensors**: `"input_0"` or `"input_1"` not found
2. **Invalid shapes**: Unexpected tensor dimensions
3. **GPU memory errors**: Insufficient GPU memory for processing
4. **Processing errors**: NMS or coordinate conversion failures

## 🚀 Performance Considerations

### GPU Acceleration
- **CuPy arrays**: All processing uses GPU memory
- **Batch processing**: Processes multiple detections simultaneously
- **Memory efficiency**: Reuses GPU memory pools

### Optimization Features
- **Confidence filtering**: Early rejection of low-confidence detections
- **NMS optimization**: Efficient non-maximum suppression
- **Coordinate conversion**: Optimized box format conversion

## 📝 Usage Examples

### Python Implementation
```python
# Create operator
postprocessor = YoloSegPostprocesserOp(
    label_dict=label_dict,
    scores_threshold=0.25,
    num_class=12
)

# Add to pipeline
pipeline.add_operator(postprocessor)
```

### C++ Implementation
```cpp
// Create operator
auto postprocessor = std::make_shared<urology::YoloSegPostprocessorOp>();

// Configure parameters
postprocessor->scores_threshold_ = 0.25;
postprocessor->num_class_ = 12;

// Add to pipeline
pipeline.add_operator(postprocessor);
```

## 🔍 Debugging

### Logging
```python
logger.info(f"output before sort {output}")
logger.info(f"output after sort {output}")
logger.info(f"out_boxes: {out_boxes}")
```

### Common Debug Information
- **Detection counts**: Number of detections per class
- **Confidence scores**: Filtered confidence values
- **Box coordinates**: Final bounding box positions
- **Processing time**: Performance metrics

## 📚 Related Components

### Dependencies
- **yolo_utils.py**: Utility functions for YOLO processing
- **log_config.py**: Logging configuration
- **inference_postproc_exceptions.py**: Custom exceptions

### Downstream Consumers
- **HolovizOp**: Visualization of detection results
- **Video Encoder**: Recording of processed frames
- **File Writer**: Saving detection results

---

This documentation provides a comprehensive overview of the YOLO segmentation postprocessor's inputs, outputs, and processing pipeline. For implementation details, refer to the source code in `urology-inference-holoscan-py/python/src/operators/yolo_seg_postprocessor.py`. 