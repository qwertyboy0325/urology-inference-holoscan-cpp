#include "operators/plug_and_play_detection.hpp"
#include "utils/logger.hpp"
#include "utils/tensor_factory.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace urology {

// Detection validation implementation
bool Detection::is_valid() const {
    if (x1 >= x2 || y1 >= y2) return false;
    if (x1 < 0.0f || y1 < 0.0f || x2 > 1.0f || y2 > 1.0f) return false;
    if (score < 0.0f || score > 1.0f) return false;
    if (class_id < 0) return false;
    if (color_r < 0.0f || color_g < 0.0f || color_b < 0.0f || color_a < 0.0f) return false;
    if (color_r > 1.0f || color_g > 1.0f || color_b > 1.0f || color_a > 1.0f) return false;
    if (text_size <= 0.0f) return false;
    return true;
}

std::string Detection::validate() const {
    if (x1 >= x2 || y1 >= y2) return "Invalid box coordinates (x1>=x2 or y1>=y2)";
    if (x1 < 0.0f || y1 < 0.0f || x2 > 1.0f || y2 > 1.0f) return "Box coordinates out of range [0,1]";
    if (score < 0.0f || score > 1.0f) return "Invalid confidence score";
    if (class_id < 0) return "Invalid class ID";
    if (color_r < 0.0f || color_g < 0.0f || color_b < 0.0f || color_a < 0.0f) return "Invalid color values (negative)";
    if (color_r > 1.0f || color_g > 1.0f || color_b > 1.0f || color_a > 1.0f) return "Invalid color values (>1)";
    if (text_size <= 0.0f) return "Invalid text size";
    return "Valid";
}

// ClassMetadata validation implementation
bool ClassMetadata::is_valid() const {
    if (class_id < 0) return false;
    if (name.empty()) return false;
    if (color.size() != 4) return false;
    for (float c : color) {
        if (c < 0.0f || c > 1.0f) return false;
    }
    return true;
}

std::string ClassMetadata::validate() const {
    if (class_id < 0) return "Invalid class ID";
    if (name.empty()) return "Empty class name";
    if (color.size() != 4) return "Invalid color size (expected 4 RGBA values)";
    for (size_t i = 0; i < color.size(); ++i) {
        if (color[i] < 0.0f || color[i] > 1.0f) {
            return "Invalid color value at index " + std::to_string(i);
        }
    }
    return "Valid";
}

std::string ClassMetadata::to_json() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"class_id\":" << class_id << ",";
    oss << "\"name\":\"" << name << "\",";
    oss << "\"color\":[" << color[0] << "," << color[1] << "," << color[2] << "," << color[3] << "],";
    oss << "\"is_active\":" << (is_active ? "true" : "false");
    oss << "}";
    return oss.str();
}

ClassMetadata ClassMetadata::from_json(const std::string& json) {
    // Simple JSON parsing - in production, use a proper JSON library
    ClassMetadata metadata;
    // TODO: Implement proper JSON parsing
    return metadata;
}

// DetectionTensorMap validation implementation
bool DetectionTensorMap::is_valid() const {
    if (!detections) return false;
    if (num_detections < 0) return false;
    if (num_classes < 0) return false;
    
    auto det_shape = detections->shape();
    if (det_shape.size() != 2) return false;
    if (det_shape[1] != DETECTION_FIELD_COUNT) return false;
    if (det_shape[0] != num_detections) return false;
    
    if (masks && num_detections > 0) {
        auto mask_shape = masks->shape();
        if (mask_shape.size() != 3) return false;
        if (mask_shape[0] != num_detections) return false;
    }
    
    if (labels && num_detections > 0) {
        auto label_shape = labels->shape();
        if (label_shape.size() != 2) return false;
        if (label_shape[0] != num_detections) return false;
    }
    
    return true;
}

std::string DetectionTensorMap::validate() const {
    if (!detections) return "Missing detections tensor";
    if (num_detections < 0) return "Invalid number of detections";
    if (num_classes < 0) return "Invalid number of classes";
    
    auto det_shape = detections->shape();
    if (det_shape.size() != 2) return "Invalid detection tensor dimensions";
    if (det_shape[1] != DETECTION_FIELD_COUNT) return "Invalid detection tensor field count";
    if (det_shape[0] != num_detections) return "Detection count mismatch";
    
    if (masks && num_detections > 0) {
        auto mask_shape = masks->shape();
        if (mask_shape.size() != 3) return "Invalid mask tensor dimensions";
        if (mask_shape[0] != num_detections) return "Mask count mismatch";
    }
    
    if (labels && num_detections > 0) {
        auto label_shape = labels->shape();
        if (label_shape.size() != 2) return "Invalid label tensor dimensions";
        if (label_shape[0] != num_detections) return "Label count mismatch";
    }
    
    return "Valid";
}

std::string DetectionTensorMap::to_json() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"frame_id\":" << frame_id << ",";
    oss << "\"timestamp\":" << timestamp << ",";
    oss << "\"num_detections\":" << num_detections << ",";
    oss << "\"num_classes\":" << num_classes << ",";
    oss << "\"has_masks\":" << (has_masks() ? "true" : "false") << ",";
    oss << "\"has_labels\":" << (has_labels() ? "true" : "false") << ",";
    oss << "\"metadata\":{";
    bool first = true;
    for (const auto& [key, value] : metadata) {
        if (!first) oss << ",";
        oss << "\"" << key << "\":\"" << value << "\"";
        first = false;
    }
    oss << "}";
    oss << "}";
    return oss.str();
}

DetectionTensorMap DetectionTensorMap::from_json(const std::string& json) {
    // Simple JSON parsing - in production, use a proper JSON library
    DetectionTensorMap tensor_map;
    // TODO: Implement proper JSON parsing
    return tensor_map;
}

std::vector<Detection> DetectionTensorMap::get_detections() const {
    return plug_and_play_utils::extract_detections_from_tensor(detections);
}

std::vector<ClassMetadata> DetectionTensorMap::get_class_metadata() const {
    // TODO: Implement class metadata extraction
    return std::vector<ClassMetadata>();
}

void DetectionTensorMap::set_detections(const std::vector<Detection>& detections) {
    this->detections = plug_and_play_utils::create_detection_tensor(detections);
    this->num_detections = detections.size();
}

void DetectionTensorMap::set_class_metadata(const std::vector<ClassMetadata>& classes) {
    this->num_classes = classes.size();
    // TODO: Implement class metadata tensor creation
}

size_t DetectionTensorMap::get_detection_count() const {
    if (!detections) return 0;
    auto shape = detections->shape();
    if (shape.size() != 2) return 0;
    return static_cast<size_t>(shape[0]);
}

size_t DetectionTensorMap::get_class_count() const {
    return static_cast<size_t>(num_classes);
}

// Utility functions implementation
namespace plug_and_play_utils {

std::shared_ptr<holoscan::Tensor> create_detection_tensor(
    const std::vector<Detection>& detections) {
    
    if (detections.empty()) {
        logger::warn("Creating empty detection tensor");
        // Create empty tensor with shape [1, 16] filled with zeros
        std::vector<float> empty_data(DETECTION_FIELD_COUNT, 0.0f);
        std::vector<int64_t> shape = {1, DETECTION_FIELD_COUNT};
        
        tensor_factory::TensorOptions options;
        options.dtype = tensor_factory::TensorType::FLOAT32;
        options.name = "empty_detections";
        
        return tensor_factory::TensorFactory::create_holoviz_tensor(
            empty_data, shape, options);
    }
    
    // Create detection data array
    std::vector<float> detection_data;
    detection_data.reserve(detections.size() * DETECTION_FIELD_COUNT);
    
    for (const auto& detection : detections) {
        // Validate detection before adding
        if (!detection.is_valid()) {
            logger::warn("Skipping invalid detection: {}", detection.validate());
            continue;
        }
        
        // Add all 16 fields in order
        detection_data.push_back(detection.x1);           // 0: X1
        detection_data.push_back(detection.y1);           // 1: Y1
        detection_data.push_back(detection.x2);           // 2: X2
        detection_data.push_back(detection.y2);           // 3: Y2
        detection_data.push_back(detection.score);        // 4: SCORE
        detection_data.push_back(static_cast<float>(detection.class_id)); // 5: CLASS_ID
        detection_data.push_back(0.0f);                   // 6: MASK_OFFSET (placeholder)
        detection_data.push_back(0.0f);                   // 7: LABEL_OFFSET (placeholder)
        detection_data.push_back(detection.color_r);      // 8: COLOR_R
        detection_data.push_back(detection.color_g);      // 9: COLOR_G
        detection_data.push_back(detection.color_b);      // 10: COLOR_B
        detection_data.push_back(detection.color_a);      // 11: COLOR_A
        detection_data.push_back(detection.text_x);       // 12: TEXT_X
        detection_data.push_back(detection.text_y);       // 13: TEXT_Y
        detection_data.push_back(detection.text_size);    // 14: TEXT_SIZE
        detection_data.push_back(0.0f);                   // 15: RESERVED
    }
    
    // Create tensor shape
    std::vector<int64_t> shape = {static_cast<int64_t>(detections.size()), DETECTION_FIELD_COUNT};
    
    // Create tensor using factory
    tensor_factory::TensorOptions options;
    options.dtype = tensor_factory::TensorType::FLOAT32;
    options.name = "detections";
    
    logger::debug("Created detection tensor with shape [{}, {}] and {} detections", 
                  shape[0], shape[1], detections.size());
    
    return tensor_factory::TensorFactory::create_holoviz_tensor(
        detection_data, shape, options);
}

std::shared_ptr<holoscan::Tensor> create_masks_tensor(
    const std::vector<Detection>& detections) {
    
    if (detections.empty()) {
        logger::debug("No detections provided for mask tensor creation");
        return nullptr;
    }
    
    // Check if any detections have masks
    bool has_masks = false;
    size_t mask_size = 0;
    
    for (const auto& detection : detections) {
        if (!detection.mask.empty()) {
            has_masks = true;
            if (mask_size == 0) {
                mask_size = detection.mask.size();
            } else if (detection.mask.size() != mask_size) {
                logger::warn("Inconsistent mask sizes detected");
                return nullptr;
            }
        }
    }
    
    if (!has_masks) {
        logger::debug("No masks found in detections");
        return nullptr;
    }
    
    // Calculate mask dimensions (assuming square masks)
    size_t mask_dim = static_cast<size_t>(std::sqrt(mask_size));
    if (mask_dim * mask_dim != mask_size) {
        logger::error("Non-square mask detected, size: {}", mask_size);
        return nullptr;
    }
    
    // Create mask data array
    std::vector<float> mask_data;
    mask_data.reserve(detections.size() * mask_size);
    
    for (const auto& detection : detections) {
        if (detection.mask.empty()) {
            // Fill with zeros for detections without masks
            mask_data.insert(mask_data.end(), mask_size, 0.0f);
        } else {
            // Add mask data
            mask_data.insert(mask_data.end(), detection.mask.begin(), detection.mask.end());
        }
    }
    
    // Create tensor shape
    std::vector<int64_t> shape = {
        static_cast<int64_t>(detections.size()),
        static_cast<int64_t>(mask_dim),
        static_cast<int64_t>(mask_dim)
    };
    
    // Create tensor using factory
    tensor_factory::TensorOptions options;
    options.dtype = tensor_factory::TensorType::FLOAT32;
    options.name = "masks";
    
    logger::debug("Created mask tensor with shape [{}, {}, {}]", 
                  shape[0], shape[1], shape[2]);
    
    return tensor_factory::TensorFactory::create_holoviz_tensor(
        mask_data, shape, options);
}

std::shared_ptr<holoscan::Tensor> create_labels_tensor(
    const std::vector<Detection>& detections,
    size_t max_label_length) {
    
    if (detections.empty()) {
        logger::debug("No detections provided for label tensor creation");
        return nullptr;
    }
    
    // Check if any detections have labels
    bool has_labels = false;
    for (const auto& detection : detections) {
        if (!detection.label.empty()) {
            has_labels = true;
            break;
        }
    }
    
    if (!has_labels) {
        logger::debug("No labels found in detections");
        return nullptr;
    }
    
    // Create label data array (UTF-8 encoded)
    std::vector<uint8_t> label_data;
    label_data.reserve(detections.size() * max_label_length);
    
    for (const auto& detection : detections) {
        std::string label = detection.label;
        if (label.length() > max_label_length) {
            label = label.substr(0, max_label_length);
        }
        
        // Pad with null bytes
        size_t current_length = label.length();
        for (size_t i = 0; i < current_length; ++i) {
            label_data.push_back(static_cast<uint8_t>(label[i]));
        }
        for (size_t i = current_length; i < max_label_length; ++i) {
            label_data.push_back(0); // null byte
        }
    }
    
    // Create tensor shape
    std::vector<int64_t> shape = {
        static_cast<int64_t>(detections.size()),
        static_cast<int64_t>(max_label_length)
    };
    
    // Create tensor using factory
    tensor_factory::TensorOptions options;
    options.dtype = tensor_factory::TensorType::UINT8;
    options.name = "labels";
    
    logger::debug("Created label tensor with shape [{}, {}]", 
                  shape[0], shape[1]);
    
    return tensor_factory::TensorFactory::create_holoviz_tensor(
        label_data, shape, options);
}

DetectionTensorMap create_detection_tensor_map(
    const std::vector<Detection>& detections,
    int64_t frame_id,
    int64_t timestamp) {
    
    DetectionTensorMap tensor_map;
    
    // Set metadata
    tensor_map.frame_id = frame_id;
    tensor_map.timestamp = timestamp;
    tensor_map.num_detections = detections.size();
    
    // Create detection tensor
    tensor_map.detections = create_detection_tensor(detections);
    
    // Create optional masks tensor
    tensor_map.masks = create_masks_tensor(detections);
    
    // Create optional labels tensor
    tensor_map.labels = create_labels_tensor(detections);
    
    // Add processing metadata
    tensor_map.metadata["processing_time"] = std::to_string(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count());
    tensor_map.metadata["version"] = "1.0";
    tensor_map.metadata["format"] = "plug_and_play";
    
    logger::debug("Created detection tensor map with {} detections", detections.size());
    
    return tensor_map;
}

std::vector<Detection> extract_detections_from_tensor(
    const std::shared_ptr<holoscan::Tensor>& tensor) {
    
    std::vector<Detection> detections;
    
    if (!tensor) {
        logger::warn("Null tensor provided for detection extraction");
        return detections;
    }
    
    auto shape = tensor->shape();
    if (shape.size() != 2) {
        logger::error("Invalid tensor shape for detection extraction: expected 2D, got {}D", shape.size());
        return detections;
    }
    
    if (shape[1] != DETECTION_FIELD_COUNT) {
        logger::error("Invalid tensor field count: expected {}, got {}", DETECTION_FIELD_COUNT, shape[1]);
        return detections;
    }
    
    int64_t num_detections = shape[0];
    detections.reserve(num_detections);
    
    // Get tensor data
    auto* data = static_cast<const float*>(tensor->data());
    if (!data) {
        logger::error("Failed to access tensor data");
        return detections;
    }
    
    // Extract detections
    for (int64_t i = 0; i < num_detections; ++i) {
        Detection detection;
        
        int64_t offset = i * DETECTION_FIELD_COUNT;
        
        detection.x1 = data[offset + static_cast<int32_t>(DetectionField::X1)];
        detection.y1 = data[offset + static_cast<int32_t>(DetectionField::Y1)];
        detection.x2 = data[offset + static_cast<int32_t>(DetectionField::X2)];
        detection.y2 = data[offset + static_cast<int32_t>(DetectionField::Y2)];
        detection.score = data[offset + static_cast<int32_t>(DetectionField::SCORE)];
        detection.class_id = static_cast<int32_t>(data[offset + static_cast<int32_t>(DetectionField::CLASS_ID)]);
        detection.color_r = data[offset + static_cast<int32_t>(DetectionField::COLOR_R)];
        detection.color_g = data[offset + static_cast<int32_t>(DetectionField::COLOR_G)];
        detection.color_b = data[offset + static_cast<int32_t>(DetectionField::COLOR_B)];
        detection.color_a = data[offset + static_cast<int32_t>(DetectionField::COLOR_A)];
        detection.text_x = data[offset + static_cast<int32_t>(DetectionField::TEXT_X)];
        detection.text_y = data[offset + static_cast<int32_t>(DetectionField::TEXT_Y)];
        detection.text_size = data[offset + static_cast<int32_t>(DetectionField::TEXT_SIZE)];
        
        detections.push_back(detection);
    }
    
    logger::debug("Extracted {} detections from tensor", detections.size());
    return detections;
}

std::vector<std::vector<float>> extract_masks_from_tensor(
    const std::shared_ptr<holoscan::Tensor>& tensor) {
    
    std::vector<std::vector<float>> masks;
    
    if (!tensor) {
        logger::warn("Null tensor provided for mask extraction");
        return masks;
    }
    
    auto shape = tensor->shape();
    if (shape.size() != 3) {
        logger::error("Invalid tensor shape for mask extraction: expected 3D, got {}D", shape.size());
        return masks;
    }
    
    int64_t num_detections = shape[0];
    int64_t mask_height = shape[1];
    int64_t mask_width = shape[2];
    int64_t mask_size = mask_height * mask_width;
    
    masks.reserve(num_detections);
    
    // Get tensor data
    auto* data = static_cast<const float*>(tensor->data());
    if (!data) {
        logger::error("Failed to access tensor data");
        return masks;
    }
    
    // Extract masks
    for (int64_t i = 0; i < num_detections; ++i) {
        std::vector<float> mask;
        mask.reserve(mask_size);
        
        int64_t offset = i * mask_size;
        for (int64_t j = 0; j < mask_size; ++j) {
            mask.push_back(data[offset + j]);
        }
        
        masks.push_back(mask);
    }
    
    logger::debug("Extracted {} masks from tensor", masks.size());
    return masks;
}

std::vector<std::string> extract_labels_from_tensor(
    const std::shared_ptr<holoscan::Tensor>& tensor) {
    
    std::vector<std::string> labels;
    
    if (!tensor) {
        logger::warn("Null tensor provided for label extraction");
        return labels;
    }
    
    auto shape = tensor->shape();
    if (shape.size() != 2) {
        logger::error("Invalid tensor shape for label extraction: expected 2D, got {}D", shape.size());
        return labels;
    }
    
    int64_t num_detections = shape[0];
    int64_t max_label_length = shape[1];
    
    labels.reserve(num_detections);
    
    // Get tensor data
    auto* data = static_cast<const uint8_t*>(tensor->data());
    if (!data) {
        logger::error("Failed to access tensor data");
        return labels;
    }
    
    // Extract labels
    for (int64_t i = 0; i < num_detections; ++i) {
        std::string label;
        int64_t offset = i * max_label_length;
        
        for (int64_t j = 0; j < max_label_length; ++j) {
            uint8_t byte = data[offset + j];
            if (byte == 0) break; // null terminator
            label += static_cast<char>(byte);
        }
        
        labels.push_back(label);
    }
    
    logger::debug("Extracted {} labels from tensor", labels.size());
    return labels;
}

std::string validate_detection_tensor(
    const std::shared_ptr<holoscan::Tensor>& tensor) {
    
    if (!tensor) return "Null tensor";
    
    auto shape = tensor->shape();
    if (shape.size() != 2) return "Invalid dimensions (expected 2D)";
    if (shape[1] != DETECTION_FIELD_COUNT) return "Invalid field count";
    if (shape[0] < 0) return "Invalid detection count";
    
    return "Valid";
}

std::string validate_masks_tensor(
    const std::shared_ptr<holoscan::Tensor>& tensor,
    int64_t num_detections) {
    
    if (!tensor) return "Null tensor";
    
    auto shape = tensor->shape();
    if (shape.size() != 3) return "Invalid dimensions (expected 3D)";
    if (shape[0] != num_detections) return "Detection count mismatch";
    if (shape[1] <= 0 || shape[2] <= 0) return "Invalid mask dimensions";
    
    return "Valid";
}

std::string validate_labels_tensor(
    const std::shared_ptr<holoscan::Tensor>& tensor,
    int64_t num_detections) {
    
    if (!tensor) return "Null tensor";
    
    auto shape = tensor->shape();
    if (shape.size() != 2) return "Invalid dimensions (expected 2D)";
    if (shape[0] != num_detections) return "Detection count mismatch";
    if (shape[1] <= 0) return "Invalid label length";
    
    return "Valid";
}

float calculate_iou(const Detection& det1, const Detection& det2) {
    // Calculate intersection
    float x1 = std::max(det1.x1, det2.x1);
    float y1 = std::max(det1.y1, det2.y1);
    float x2 = std::min(det1.x2, det2.x2);
    float y2 = std::min(det1.y2, det2.y2);
    
    if (x2 <= x1 || y2 <= y1) return 0.0f;
    
    float intersection = (x2 - x1) * (y2 - y1);
    float area1 = det1.get_area();
    float area2 = det2.get_area();
    float union_area = area1 + area2 - intersection;
    
    return intersection / union_area;
}

std::vector<Detection> apply_nms(
    const std::vector<Detection>& detections,
    float iou_threshold) {
    
    if (detections.size() <= 1) return detections;
    
    std::vector<Detection> result;
    std::vector<bool> keep(detections.size(), true);
    
    // Sort by confidence (descending)
    std::vector<size_t> indices(detections.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&detections](size_t a, size_t b) {
        return detections[a].score > detections[b].score;
    });
    
    // Apply NMS
    for (size_t i = 0; i < indices.size(); ++i) {
        if (!keep[indices[i]]) continue;
        
        result.push_back(detections[indices[i]]);
        
        for (size_t j = i + 1; j < indices.size(); ++j) {
            if (!keep[indices[j]]) continue;
            
            // Check if same class
            if (detections[indices[i]].class_id != detections[indices[j]].class_id) {
                continue;
            }
            
            // Calculate IoU
            float iou = calculate_iou(detections[indices[i]], detections[indices[j]]);
            
            if (iou > iou_threshold) {
                keep[indices[j]] = false;
            }
        }
    }
    
    logger::debug("Applied NMS: {} -> {} detections", detections.size(), result.size());
    return result;
}

std::vector<Detection> filter_by_confidence(
    const std::vector<Detection>& detections,
    float confidence_threshold) {
    
    std::vector<Detection> result;
    result.reserve(detections.size());
    
    for (const auto& detection : detections) {
        if (detection.score >= confidence_threshold) {
            result.push_back(detection);
        }
    }
    
    logger::debug("Filtered by confidence: {} -> {} detections", detections.size(), result.size());
    return result;
}

void sort_by_confidence(std::vector<Detection>& detections) {
    std::sort(detections.begin(), detections.end(), 
              [](const Detection& a, const Detection& b) {
                  return a.score > b.score;
              });
}

void sort_by_class_id(std::vector<Detection>& detections) {
    std::sort(detections.begin(), detections.end(), 
              [](const Detection& a, const Detection& b) {
                  return a.class_id < b.class_id;
              });
}

} // namespace plug_and_play_utils

} // namespace urology 