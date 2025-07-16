# HolovizOp Compatibility Analysis & Revised Implementation

## 🚨 Critical Finding: Format Incompatibility

Our proposed plug-and-play unified detection tensor format is **NOT directly supported** by HolovizOp. This requires a significant revision to our implementation approach.

## 📋 HolovizOp Supported Formats

### 1. Static Tensor Configuration
```python
holoviz_tensors = [
    dict(name="", type="color"),           # Video stream
    dict(name="masks", type="color_lut"),  # Segmentation masks
    dict(name="boxes1", type="rectangles", opacity=0.7, line_width=4, color=color),
    dict(name="label1", type="text", opacity=0.7, color=color, text=text)
]
```

### 2. Dynamic InputSpec Configuration
```cpp
holoscan::ops::HolovizOp::InputSpec spec(score_key, "text");
spec.color_ = {1.0f, 1.0f, 1.0f, 1.0f};
specs.push_back(spec);
```

### Supported Tensor Types
- `color` - RGB/RGBA image data
- `color_lut` - Indexed color data with lookup table
- `rectangles` - Bounding boxes
- `text` - Text overlays
- `points`, `lines`, `line_strip`, `triangles`
- `crosses`, `ovals`
- `points_3d`, `lines_3d`, `line_strip_3d`, `triangles_3d`

## 🔄 Revised Implementation Strategy

### Option 1: HolovizOp-Native Multi-Tensor Output (RECOMMENDED)

Instead of a single unified tensor, create multiple tensors that HolovizOp can consume directly:

```cpp
struct HolovizCompatibleOutput {
    // Video stream (existing)
    std::shared_ptr<holoscan::Tensor> video_tensor;
    
    // Detection results (new structured format)
    std::map<std::string, std::shared_ptr<holoscan::Tensor>> detection_tensors;
    
    // Visualization specs (existing)
    std::vector<holoscan::ops::HolovizOp::InputSpec> input_specs;
};
```

**Tensor Structure**:
```cpp
// For each detection class
detection_tensors["boxes1"] = create_rectangle_tensor(boxes, class_1_color);
detection_tensors["label1"] = create_text_tensor(labels, class_1_color);
detection_tensors["score1"] = create_text_tensor(scores, class_1_color);
detection_tensors["masks"] = create_color_lut_tensor(masks, color_lut);
```

### Option 2: HolovizOp InputSpec-Only Approach

Generate only InputSpec objects and let HolovizOp handle tensor creation:

```cpp
std::vector<holoscan::ops::HolovizOp::InputSpec> create_holoviz_specs(
    const std::vector<Detection>& detections,
    const std::map<int, LabelInfo>& label_dict) {
    
    std::vector<holoscan::ops::HolovizOp::InputSpec> specs;
    
    for (const auto& detection : detections) {
        // Create rectangle spec for bounding box
        holoscan::ops::HolovizOp::InputSpec box_spec(
            "boxes" + std::to_string(detection.class_id), "rectangles");
        box_spec.color_ = label_dict[detection.class_id].color;
        box_spec.opacity_ = 0.7f;
        box_spec.line_width_ = 4;
        specs.push_back(box_spec);
        
        // Create text spec for label
        holoscan::ops::HolovizOp::InputSpec label_spec(
            "label" + std::to_string(detection.class_id), "text");
        label_spec.color_ = label_dict[detection.class_id].color;
        label_spec.text_ = {label_dict[detection.class_id].text};
        specs.push_back(label_spec);
        
        // Create text spec for score
        holoscan::ops::HolovizOp::InputSpec score_spec(
            "score" + std::to_string(detection.class_id), "text");
        score_spec.color_ = {1.0f, 1.0f, 1.0f, 1.0f}; // White
        score_spec.text_ = {std::to_string(detection.confidence)};
        specs.push_back(score_spec);
    }
    
    return specs;
}
```

### Option 3: Custom Visualization Operator

Create a custom operator that converts our unified format to HolovizOp-compatible tensors:

```cpp
class HolovizAdapterOp : public holoscan::Operator {
public:
    void setup(holoscan::OperatorSpec& spec) override {
        spec.input<UnifiedDetectionTensor>("unified_input");
        spec.output<std::map<std::string, std::shared_ptr<holoscan::Tensor>>>("holoviz_tensors");
        spec.output<std::vector<holoscan::ops::HolovizOp::InputSpec>>("input_specs");
    }
    
    void compute(holoscan::InputContext& op_input, 
                holoscan::OutputContext& op_output,
                holoscan::ExecutionContext&) override {
        // Convert unified tensor to HolovizOp-compatible format
        auto unified_tensor = op_input.receive<UnifiedDetectionTensor>("unified_input");
        auto holoviz_tensors = convert_to_holoviz_format(unified_tensor);
        auto input_specs = create_input_specs(unified_tensor);
        
        op_output.emit(holoviz_tensors, "holoviz_tensors");
        op_output.emit(input_specs, "input_specs");
    }
};
```

## 🎯 Recommended Implementation Plan

### Phase 1: HolovizOp-Native Multi-Tensor Output (Immediate)

1. **Modify plug-and-play postprocessor** to output multiple tensors
2. **Create tensor factory functions** for each HolovizOp type
3. **Generate InputSpec objects** dynamically
4. **Test with existing HolovizOp** configuration

### Phase 2: Optimization & Abstraction (Future)

1. **Create HolovizAdapterOp** for unified format conversion
2. **Implement zero-copy optimizations**
3. **Add support for custom visualization types**

## 📊 Implementation Comparison

| Approach | Complexity | Performance | Maintainability | HolovizOp Compatibility |
|----------|------------|-------------|-----------------|-------------------------|
| Multi-Tensor Output | Medium | High | High | ✅ Full |
| InputSpec-Only | Low | Medium | Medium | ✅ Full |
| Custom Adapter | High | High | High | ✅ Full |
| Unified Tensor | Low | High | High | ❌ None |

## 🔧 Revised Implementation Steps

### Step 1: Update Plug-and-Play Detection Structures
```cpp
// Modified to support HolovizOp-compatible output
struct PlugAndPlayDetectionOutput {
    std::map<std::string, std::shared_ptr<holoscan::Tensor>> tensors;
    std::vector<holoscan::ops::HolovizOp::InputSpec> input_specs;
    
    // Helper methods
    void add_rectangle_tensor(const std::string& name, 
                             const std::vector<std::vector<float>>& boxes,
                             const std::vector<float>& color);
    void add_text_tensor(const std::string& name,
                        const std::vector<std::string>& texts,
                        const std::vector<float>& color);
    void add_mask_tensor(const std::string& name,
                        const std::vector<std::vector<float>>& masks,
                        const std::vector<std::vector<float>>& color_lut);
};
```

### Step 2: Create HolovizOp Tensor Factory
```cpp
namespace holoviz_tensor_factory {
    std::shared_ptr<holoscan::Tensor> create_rectangle_tensor(
        const std::vector<std::vector<float>>& boxes,
        const std::vector<float>& color);
    
    std::shared_ptr<holoscan::Tensor> create_text_tensor(
        const std::vector<std::string>& texts,
        const std::vector<float>& color);
    
    std::shared_ptr<holoscan::Tensor> create_color_lut_tensor(
        const std::vector<std::vector<float>>& masks,
        const std::vector<std::vector<float>>& color_lut);
}
```

### Step 3: Update Postprocessor Output
```cpp
void PlugAndPlayYoloPostprocessorOp::compute(...) {
    // Process detections
    auto detections = process_detections(predictions, masks);
    
    // Create HolovizOp-compatible output
    PlugAndPlayDetectionOutput output;
    
    for (const auto& detection : detections) {
        std::string class_suffix = std::to_string(detection.class_id);
        
        // Add bounding box tensor
        output.add_rectangle_tensor("boxes" + class_suffix, 
                                   {detection.box}, 
                                   label_dict[detection.class_id].color);
        
        // Add label tensor
        output.add_text_tensor("label" + class_suffix,
                              {label_dict[detection.class_id].text},
                              label_dict[detection.class_id].color);
        
        // Add score tensor
        output.add_text_tensor("score" + class_suffix,
                              {std::to_string(detection.confidence)},
                              {1.0f, 1.0f, 1.0f, 1.0f});
    }
    
    // Add mask tensor
    if (!detections.empty()) {
        output.add_mask_tensor("masks", detection_masks, color_lut);
    }
    
    // Emit outputs
    op_output.emit(output.tensors, "out");
    op_output.emit(output.input_specs, "output_specs");
}
```

## ✅ Conclusion

While our original unified tensor approach is not directly supported by HolovizOp, we can achieve the same goals by:

1. **Creating multiple HolovizOp-compatible tensors** instead of one unified tensor
2. **Using the existing InputSpec system** for dynamic configuration
3. **Maintaining the plug-and-play philosophy** through structured output generation

This approach provides:
- ✅ **Full HolovizOp compatibility**
- ✅ **Zero-config integration**
- ✅ **High performance**
- ✅ **Maintainable code structure**

The implementation will be slightly more complex than the original unified tensor approach, but it will work seamlessly with HolovizOp's existing architecture. 