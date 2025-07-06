#pragma once

#include <vector>
#include <memory>
#include <holoscan/core/domain/tensor.hpp>

namespace urology {
namespace yolo_utils {

// Non-Maximum Suppression
std::vector<int> non_max_suppression(
    const std::vector<std::vector<float>>& boxes,
    const std::vector<float>& scores,
    float iou_threshold = 0.5f);

// Intersection over Union calculation
float calculate_iou(const std::vector<float>& box1, const std::vector<float>& box2);

// Convert between different box formats
std::vector<float> xywh_to_xyxy(const std::vector<float>& xywh);
std::vector<float> xyxy_to_xywh(const std::vector<float>& xyxy);

// Scale boxes to different resolutions
std::vector<std::vector<float>> scale_boxes(
    const std::vector<std::vector<float>>& boxes,
    float scale_x, float scale_y);

// Process YOLO model outputs
struct YoloDetection {
    std::vector<float> box;  // [x1, y1, x2, y2]
    float confidence;
    int class_id;
    std::vector<float> mask;  // segmentation mask (optional)
};

std::vector<YoloDetection> process_yolo_output(
    const std::shared_ptr<holoscan::Tensor>& predictions,
    const std::shared_ptr<holoscan::Tensor>& masks,
    float conf_threshold = 0.25f,
    float iou_threshold = 0.45f,
    int num_classes = 12);

// Mask processing utilities
std::vector<std::vector<float>> extract_masks(
    const std::shared_ptr<holoscan::Tensor>& mask_tensor,
    const std::vector<YoloDetection>& detections,
    int original_width, int original_height);

// Resize and normalize masks
std::vector<float> resize_mask(
    const std::vector<float>& mask,
    int src_width, int src_height,
    int dst_width, int dst_height);

} // namespace yolo_utils
} // namespace urology 