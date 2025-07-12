#include "utils/yolo_utils.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <holoscan/core/domain/tensor.hpp>
#include <dlpack/dlpack.h>
#include <cstring>
#include <iostream>

namespace urology {
namespace yolo_utils {

float calculate_iou(const std::vector<float>& box1, const std::vector<float>& box2) {
    // box format: [x1, y1, x2, y2]
    float x1 = std::max(box1[0], box2[0]);
    float y1 = std::max(box1[1], box2[1]);
    float x2 = std::min(box1[2], box2[2]);
    float y2 = std::min(box1[3], box2[3]);
    
    if (x2 <= x1 || y2 <= y1) {
        return 0.0f;
    }
    
    float intersection = (x2 - x1) * (y2 - y1);
    
    float area1 = (box1[2] - box1[0]) * (box1[3] - box1[1]);
    float area2 = (box2[2] - box2[0]) * (box2[3] - box2[1]);
    float union_area = area1 + area2 - intersection;
    
    return intersection / union_area;
}

std::vector<int> non_max_suppression(
    const std::vector<std::vector<float>>& boxes,
    const std::vector<float>& scores,
    float iou_threshold) {
    
    std::vector<int> indices(boxes.size());
    std::iota(indices.begin(), indices.end(), 0);
    
    // Sort by scores in descending order
    std::sort(indices.begin(), indices.end(), [&scores](int a, int b) {
        return scores[a] > scores[b];
    });
    
    std::vector<bool> suppressed(boxes.size(), false);
    std::vector<int> keep;
    
    for (int i = 0; i < indices.size(); ++i) {
        int idx = indices[i];
        if (suppressed[idx]) continue;
        
        keep.push_back(idx);
        
        // Suppress overlapping boxes
        for (int j = i + 1; j < indices.size(); ++j) {
            int other_idx = indices[j];
            if (suppressed[other_idx]) continue;
            
            float iou = calculate_iou(boxes[idx], boxes[other_idx]);
            if (iou > iou_threshold) {
                suppressed[other_idx] = true;
            }
        }
    }
    
    return keep;
}

std::vector<float> xywh_to_xyxy(const std::vector<float>& xywh) {
    // Convert from [x_center, y_center, width, height] to [x1, y1, x2, y2]
    float x_center = xywh[0];
    float y_center = xywh[1];
    float width = xywh[2];
    float height = xywh[3];
    
    return {
        x_center - width / 2.0f,   // x1
        y_center - height / 2.0f,  // y1
        x_center + width / 2.0f,   // x2
        y_center + height / 2.0f   // y2
    };
}

std::vector<float> xyxy_to_xywh(const std::vector<float>& xyxy) {
    // Convert from [x1, y1, x2, y2] to [x_center, y_center, width, height]
    float x1 = xyxy[0];
    float y1 = xyxy[1];
    float x2 = xyxy[2];
    float y2 = xyxy[3];
    
    return {
        (x1 + x2) / 2.0f,  // x_center
        (y1 + y2) / 2.0f,  // y_center
        x2 - x1,           // width
        y2 - y1            // height
    };
}

std::vector<std::vector<float>> scale_boxes(
    const std::vector<std::vector<float>>& boxes,
    float scale_x, float scale_y) {
    
    std::vector<std::vector<float>> scaled_boxes;
    for (const auto& box : boxes) {
        std::vector<float> scaled_box = {
            box[0] * scale_x,  // x1
            box[1] * scale_y,  // y1
            box[2] * scale_x,  // x2
            box[3] * scale_y   // y2
        };
        scaled_boxes.push_back(scaled_box);
    }
    
    return scaled_boxes;
}

std::vector<YoloDetection> process_yolo_output(
    const std::shared_ptr<holoscan::Tensor>& predictions,
    const std::shared_ptr<holoscan::Tensor>& masks,
    float conf_threshold,
    float iou_threshold,
    int num_classes) {
    
    std::vector<YoloDetection> detections;
    
    // Get tensor data
    const float* pred_data = static_cast<const float*>(predictions->data());
    auto pred_shape = predictions->shape();
    
    // Assuming predictions shape is [batch, num_boxes, 5 + num_classes]
    // where 5 = [x, y, w, h, confidence]
    if (pred_shape.size() < 3) {
        return detections; // Invalid shape
    }
    
    int batch_size = pred_shape[0];
    int num_boxes = pred_shape[1];
    int box_size = pred_shape[2];
    
    std::vector<std::vector<float>> boxes;
    std::vector<float> scores;
    std::vector<int> class_ids;
    
    // Extract detections
    for (int i = 0; i < num_boxes; ++i) {
        const float* box_data = pred_data + i * box_size;
        
        // Extract coordinates (assuming YOLO format: x, y, w, h)
        float x = box_data[0];
        float y = box_data[1];
        float w = box_data[2];
        float h = box_data[3];
        float obj_conf = box_data[4];
        
        // Find best class
        int best_class = 0;
        float best_class_conf = box_data[5];
        for (int c = 1; c < num_classes; ++c) {
            if (box_data[5 + c] > best_class_conf) {
                best_class_conf = box_data[5 + c];
                best_class = c;
            }
        }
        
        float final_conf = obj_conf * best_class_conf;
        if (final_conf < conf_threshold) continue;
        
        // Convert to xyxy format
        auto xyxy = xywh_to_xyxy({x, y, w, h});
        
        boxes.push_back(xyxy);
        scores.push_back(final_conf);
        class_ids.push_back(best_class);
    }
    
    // Apply NMS
    auto keep_indices = non_max_suppression(boxes, scores, iou_threshold);
    
    // Create final detections
    for (int idx : keep_indices) {
        YoloDetection detection;
        detection.box = boxes[idx];
        detection.confidence = scores[idx];
        detection.class_id = class_ids[idx];
        
        // Extract mask if available
        if (masks && masks->data()) {
            // TODO: Implement mask extraction
            // This would depend on your specific mask format
        }
        
        detections.push_back(detection);
    }
    
    return detections;
}

std::vector<std::vector<float>> extract_masks(
    const std::shared_ptr<holoscan::Tensor>& mask_tensor,
    const std::vector<YoloDetection>& detections,
    int original_width, int original_height) {
    
    std::vector<std::vector<float>> masks;
    
    if (!mask_tensor || !mask_tensor->data()) {
        return masks;
    }
    
    // TODO: Implement mask extraction based on your specific mask format
    // This is a placeholder implementation
    
    return masks;
}

std::vector<float> resize_mask(
    const std::vector<float>& mask,
    int src_width, int src_height,
    int dst_width, int dst_height) {
    
    std::vector<float> resized_mask(dst_width * dst_height);
    
    float x_ratio = static_cast<float>(src_width) / dst_width;
    float y_ratio = static_cast<float>(src_height) / dst_height;
    
    for (int y = 0; y < dst_height; ++y) {
        for (int x = 0; x < dst_width; ++x) {
            int src_x = static_cast<int>(x * x_ratio);
            int src_y = static_cast<int>(y * y_ratio);
            
            // Clamp to source bounds
            src_x = std::min(src_x, src_width - 1);
            src_y = std::min(src_y, src_height - 1);
            
            int src_idx = src_y * src_width + src_x;
            int dst_idx = y * dst_width + x;
            
            if (src_idx < mask.size()) {
                resized_mask[dst_idx] = mask[src_idx];
            }
        }
    }
    
    return resized_mask;
}

std::shared_ptr<holoscan::Tensor> make_holoscan_tensor_from_data(
    void* data, const std::vector<int64_t>& shape, int dtype_code, int dtype_bits, int dtype_lanes) {
    // Allocate and fill DLTensor
    auto* dl_managed_tensor = new DLManagedTensor();
    dl_managed_tensor->dl_tensor.data = data;
    dl_managed_tensor->dl_tensor.device = DLDevice{ kDLCPU, 0 };
    dl_managed_tensor->dl_tensor.ndim = static_cast<int>(shape.size());
    dl_managed_tensor->dl_tensor.dtype = DLDataType{ static_cast<uint8_t>(dtype_code), static_cast<uint8_t>(dtype_bits), static_cast<uint16_t>(dtype_lanes) };
    dl_managed_tensor->dl_tensor.shape = new int64_t[shape.size()];
    std::memcpy(dl_managed_tensor->dl_tensor.shape, shape.data(), shape.size() * sizeof(int64_t));
    dl_managed_tensor->dl_tensor.strides = nullptr;
    dl_managed_tensor->dl_tensor.byte_offset = 0;
    dl_managed_tensor->manager_ctx = nullptr;
    dl_managed_tensor->deleter = [](DLManagedTensor* self) {
        delete[] self->dl_tensor.shape;
        delete self;
    };

    // Wrap in Holoscan Tensor
    auto tensor = std::make_shared<holoscan::Tensor>(dl_managed_tensor);
    return tensor;
}

// Alternative function that creates tensors using a simpler approach
std::shared_ptr<holoscan::Tensor> make_holoscan_tensor_simple(
    const std::vector<float>& data, const std::vector<int64_t>& shape) {
    // Create a copy of the data to ensure it stays alive
    auto* data_copy = new std::vector<float>(data);
    
    // Allocate and fill DLTensor with float32
    auto* dl_managed_tensor = new DLManagedTensor();
    dl_managed_tensor->dl_tensor.data = data_copy->data();
    dl_managed_tensor->dl_tensor.device = DLDevice{ kDLCPU, 0 };
    dl_managed_tensor->dl_tensor.ndim = static_cast<int>(shape.size());
    dl_managed_tensor->dl_tensor.dtype = DLDataType{ 1, 32, 1 };  // kFloat32=1, 32 bits, 1 lane
    dl_managed_tensor->dl_tensor.shape = new int64_t[shape.size()];
    std::memcpy(dl_managed_tensor->dl_tensor.shape, shape.data(), shape.size() * sizeof(int64_t));
    dl_managed_tensor->dl_tensor.strides = nullptr;
    dl_managed_tensor->dl_tensor.byte_offset = 0;
    dl_managed_tensor->manager_ctx = data_copy;  // Store the data vector
    dl_managed_tensor->deleter = [](DLManagedTensor* self) {
        delete[] self->dl_tensor.shape;
        delete static_cast<std::vector<float>*>(self->manager_ctx);
        delete self;
    };

    // Wrap in Holoscan Tensor
    auto tensor = std::make_shared<holoscan::Tensor>(dl_managed_tensor);
    return tensor;
}

// Try a different approach - create tensor with different dtype mapping
std::shared_ptr<holoscan::Tensor> make_holoscan_tensor_holoviz_compatible(
    const std::vector<float>& data, const std::vector<int64_t>& shape) {
    // Create a copy of the data to ensure it stays alive
    auto* data_copy = new std::vector<float>(data);
    
    // Allocate and fill DLTensor - try using kDLInt=0 but with float data
    // This might work if HolovizOp expects a different dtype mapping
    auto* dl_managed_tensor = new DLManagedTensor();
    dl_managed_tensor->dl_tensor.data = data_copy->data();
    dl_managed_tensor->dl_tensor.device = DLDevice{ kDLCPU, 0 };
    dl_managed_tensor->dl_tensor.ndim = static_cast<int>(shape.size());
    dl_managed_tensor->dl_tensor.dtype = DLDataType{ 0, 32, 1 };  // kDLInt=0, 32 bits, 1 lane
    dl_managed_tensor->dl_tensor.shape = new int64_t[shape.size()];
    std::memcpy(dl_managed_tensor->dl_tensor.shape, shape.data(), shape.size() * sizeof(int64_t));
    dl_managed_tensor->dl_tensor.strides = nullptr;
    dl_managed_tensor->dl_tensor.byte_offset = 0;
    dl_managed_tensor->manager_ctx = data_copy;  // Store the data vector
    dl_managed_tensor->deleter = [](DLManagedTensor* self) {
        delete[] self->dl_tensor.shape;
        delete static_cast<std::vector<float>*>(self->manager_ctx);
        delete self;
    };

    // Wrap in Holoscan Tensor
    auto tensor = std::make_shared<holoscan::Tensor>(dl_managed_tensor);
    return tensor;
}

// Try using a completely different approach - create tensor without sending to HolovizOp
std::shared_ptr<holoscan::Tensor> make_holoscan_tensor_no_holoviz(
    const std::vector<float>& data, const std::vector<int64_t>& shape) {
    // For now, just return nullptr to skip tensor creation entirely
    // This will help us test if the issue is specifically with HolovizOp
    std::cout << "[TENSOR] Skipping tensor creation for HolovizOp compatibility test" << std::endl;
    return nullptr;
}

} // namespace yolo_utils
} // namespace urology 