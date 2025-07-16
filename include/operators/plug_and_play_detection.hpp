#pragma once

#include <holoscan/holoscan.hpp>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <cstdint>

namespace urology {

/**
 * @brief Detection tensor field indices for plug-and-play format
 * 
 * Each detection is represented as a row in a tensor with 16 fields.
 * This enum provides clear field mapping for easy access and maintenance.
 */
enum class DetectionField : int32_t {
    X1 = 0,           // Top-left x coordinate (normalized)
    Y1 = 1,           // Top-left y coordinate (normalized)
    X2 = 2,           // Bottom-right x coordinate (normalized)
    Y2 = 3,           // Bottom-right y coordinate (normalized)
    SCORE = 4,        // Detection confidence score
    CLASS_ID = 5,     // Class index
    MASK_OFFSET = 6,  // Offset into mask tensor (if used)
    LABEL_OFFSET = 7, // Offset into label string tensor
    COLOR_R = 8,      // Box/text color red component
    COLOR_G = 9,      // Box/text color green component
    COLOR_B = 10,     // Box/text color blue component
    COLOR_A = 11,     // Box/text color alpha component
    TEXT_X = 12,      // Text anchor x coordinate (normalized)
    TEXT_Y = 13,      // Text anchor y coordinate (normalized)
    TEXT_SIZE = 14,   // Text size (relative)
    RESERVED = 15     // Reserved for future use
};

/**
 * @brief Number of fields per detection in the plug-and-play format
 */
constexpr int32_t DETECTION_FIELD_COUNT = 16;

/**
 * @brief Individual detection structure for internal processing
 */
struct Detection {
    // Bounding box coordinates (normalized 0-1)
    float x1 = 0.0f;
    float y1 = 0.0f;
    float x2 = 0.0f;
    float y2 = 0.0f;
    
    // Detection metadata
    float score = 0.0f;
    int32_t class_id = -1;
    
    // Visualization properties
    float color_r = 1.0f;
    float color_g = 1.0f;
    float color_b = 1.0f;
    float color_a = 1.0f;
    
    // Text positioning
    float text_x = 0.0f;
    float text_y = 0.0f;
    float text_size = 0.04f;
    
    // Optional data
    std::vector<float> mask;  // Segmentation mask (if applicable)
    std::string label;        // Class label text
    
    // Validation
    bool is_valid() const;
    std::string validate() const;
    
    // Utility methods
    float get_width() const { return x2 - x1; }
    float get_height() const { return y2 - y1; }
    float get_area() const { return get_width() * get_height(); }
    float get_center_x() const { return (x1 + x2) / 2.0f; }
    float get_center_y() const { return (y1 + y2) / 2.0f; }
};

/**
 * @brief Class metadata structure for visualization
 */
struct ClassMetadata {
    int32_t class_id = -1;
    std::string name;
    std::vector<float> color = {1.0f, 1.0f, 1.0f, 1.0f};  // RGBA
    bool is_active = true;
    
    // Validation
    bool is_valid() const;
    std::string validate() const;
    
    // Serialization
    std::string to_json() const;
    static ClassMetadata from_json(const std::string& json);
};

/**
 * @brief Plug-and-play detection tensor map structure
 * 
 * This structure contains all the tensors needed for zero-config
 * visualization with HolovizOp.
 */
struct DetectionTensorMap {
    // Main detection tensor: [num_detections, 16]
    // Contains all detection information in a single, structured format
    std::shared_ptr<holoscan::Tensor> detections;
    
    // Optional masks tensor: [num_detections, mask_height, mask_width]
    // Segmentation masks for each detection (if applicable)
    std::shared_ptr<holoscan::Tensor> masks;
    
    // Optional labels tensor: [num_detections, max_label_length]
    // UTF-8 encoded label strings for each detection
    std::shared_ptr<holoscan::Tensor> labels;
    
    // Metadata
    std::map<std::string, std::string> metadata;
    int64_t frame_id = -1;
    int64_t timestamp = -1;
    int64_t num_detections = 0;
    int64_t num_classes = 0;
    
    // Validation and utilities
    bool is_valid() const;
    std::string validate() const;
    std::string to_json() const;
    static DetectionTensorMap from_json(const std::string& json);
    
    // Accessors
    std::vector<Detection> get_detections() const;
    std::vector<ClassMetadata> get_class_metadata() const;
    void set_detections(const std::vector<Detection>& detections);
    void set_class_metadata(const std::vector<ClassMetadata>& classes);
    
    // Utility methods
    bool has_masks() const { return masks != nullptr; }
    bool has_labels() const { return labels != nullptr; }
    size_t get_detection_count() const;
    size_t get_class_count() const;
};

/**
 * @brief Utility functions for plug-and-play detection tensors
 */
namespace plug_and_play_utils {

/**
 * @brief Create a detection tensor from a vector of detections
 * 
 * @param detections Vector of detection objects
 * @return std::shared_ptr<holoscan::Tensor> Detection tensor with shape [N, 16]
 */
std::shared_ptr<holoscan::Tensor> create_detection_tensor(
    const std::vector<Detection>& detections);

/**
 * @brief Create a masks tensor from detection masks
 * 
 * @param detections Vector of detection objects with masks
 * @return std::shared_ptr<holoscan::Tensor> Masks tensor with shape [N, H, W] or nullptr if no masks
 */
std::shared_ptr<holoscan::Tensor> create_masks_tensor(
    const std::vector<Detection>& detections);

/**
 * @brief Create a labels tensor from detection labels
 * 
 * @param detections Vector of detection objects with labels
 * @param max_label_length Maximum length for label strings
 * @return std::shared_ptr<holoscan::Tensor> Labels tensor with shape [N, max_label_length] or nullptr if no labels
 */
std::shared_ptr<holoscan::Tensor> create_labels_tensor(
    const std::vector<Detection>& detections,
    size_t max_label_length = 64);

/**
 * @brief Create a complete DetectionTensorMap from detections
 * 
 * @param detections Vector of detection objects
 * @param frame_id Frame identifier
 * @param timestamp Timestamp in milliseconds
 * @return DetectionTensorMap Complete tensor map ready for emission
 */
DetectionTensorMap create_detection_tensor_map(
    const std::vector<Detection>& detections,
    int64_t frame_id = -1,
    int64_t timestamp = -1);

/**
 * @brief Extract detections from a detection tensor
 * 
 * @param tensor Detection tensor with shape [N, 16]
 * @return std::vector<Detection> Vector of detection objects
 */
std::vector<Detection> extract_detections_from_tensor(
    const std::shared_ptr<holoscan::Tensor>& tensor);

/**
 * @brief Extract masks from a masks tensor
 * 
 * @param tensor Masks tensor with shape [N, H, W]
 * @return std::vector<std::vector<float>> Vector of mask vectors
 */
std::vector<std::vector<float>> extract_masks_from_tensor(
    const std::shared_ptr<holoscan::Tensor>& tensor);

/**
 * @brief Extract labels from a labels tensor
 * 
 * @param tensor Labels tensor with shape [N, L]
 * @return std::vector<std::string> Vector of label strings
 */
std::vector<std::string> extract_labels_from_tensor(
    const std::shared_ptr<holoscan::Tensor>& tensor);

/**
 * @brief Validate detection tensor format
 * 
 * @param tensor Tensor to validate
 * @return std::string Error message or empty string if valid
 */
std::string validate_detection_tensor(
    const std::shared_ptr<holoscan::Tensor>& tensor);

/**
 * @brief Validate masks tensor format
 * 
 * @param tensor Tensor to validate
 * @param num_detections Expected number of detections
 * @return std::string Error message or empty string if valid
 */
std::string validate_masks_tensor(
    const std::shared_ptr<holoscan::Tensor>& tensor,
    int64_t num_detections);

/**
 * @brief Validate labels tensor format
 * 
 * @param tensor Tensor to validate
 * @param num_detections Expected number of detections
 * @return std::string Error message or empty string if valid
 */
std::string validate_labels_tensor(
    const std::shared_ptr<holoscan::Tensor>& tensor,
    int64_t num_detections);

/**
 * @brief Calculate IoU (Intersection over Union) between two detections
 * 
 * @param det1 First detection
 * @param det2 Second detection
 * @return float IoU value between 0 and 1
 */
float calculate_iou(const Detection& det1, const Detection& det2);

/**
 * @brief Apply Non-Maximum Suppression to detections
 * 
 * @param detections Vector of detections
 * @param iou_threshold IoU threshold for suppression
 * @return std::vector<Detection> Filtered detections
 */
std::vector<Detection> apply_nms(
    const std::vector<Detection>& detections,
    float iou_threshold = 0.45f);

/**
 * @brief Filter detections by confidence threshold
 * 
 * @param detections Vector of detections
 * @param confidence_threshold Minimum confidence score
 * @return std::vector<Detection> Filtered detections
 */
std::vector<Detection> filter_by_confidence(
    const std::vector<Detection>& detections,
    float confidence_threshold = 0.2f);

/**
 * @brief Sort detections by confidence score (descending)
 * 
 * @param detections Vector of detections to sort
 */
void sort_by_confidence(std::vector<Detection>& detections);

/**
 * @brief Sort detections by class ID
 * 
 * @param detections Vector of detections to sort
 */
void sort_by_class_id(std::vector<Detection>& detections);

} // namespace plug_and_play_utils

} // namespace urology 