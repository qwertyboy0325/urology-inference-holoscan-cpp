#pragma once

#include <holoscan/holoscan.hpp>
#include <vector>
#include <string>
#include <memory>

namespace urology {
namespace holoviz_tensor_factory {

/**
 * @brief Create a rectangle tensor for bounding boxes
 * 
 * Format: [num_boxes, 4] for [x1, y1, x2, y2]
 * Data type: float32
 * 
 * @param boxes Vector of bounding boxes in [x1, y1, x2, y2] format
 * @return Shared pointer to Holoscan tensor
 */
std::shared_ptr<holoscan::Tensor> create_rectangle_tensor(
    const std::vector<std::vector<float>>& boxes);

/**
 * @brief Create a text tensor for labels or scores
 * 
 * Format: [num_texts, 3] for [x, y, size]
 * Data type: float32
 * 
 * @param positions Vector of text positions in [x, y, size] format
 * @return Shared pointer to Holoscan tensor
 */
std::shared_ptr<holoscan::Tensor> create_text_tensor(
    const std::vector<std::vector<float>>& positions);

/**
 * @brief Create a color LUT tensor for segmentation masks
 * 
 * Format: [num_masks, height, width]
 * Data type: float32
 * 
 * @param masks Vector of segmentation masks
 * @param height Mask height
 * @param width Mask width
 * @return Shared pointer to Holoscan tensor
 */
std::shared_ptr<holoscan::Tensor> create_color_lut_tensor(
    const std::vector<std::vector<float>>& masks,
    int height, int width);

/**
 * @brief Create a color tensor for video streams
 * 
 * Format: [height, width, channels] or [channels, height, width]
 * Data type: float32 or uint8
 * 
 * @param data Raw image data
 * @param height Image height
 * @param width Image width
 * @param channels Number of channels (3 for RGB, 4 for RGBA)
 * @param data_type Data type ("float32" or "uint8")
 * @param layout Data layout ("hwc" for height-width-channels, "chw" for channels-height-width)
 * @return Shared pointer to Holoscan tensor
 */
std::shared_ptr<holoscan::Tensor> create_color_tensor(
    const std::vector<uint8_t>& data,
    int height, int width, int channels,
    const std::string& data_type = "uint8",
    const std::string& layout = "hwc");

/**
 * @brief Create a points tensor for 2D points
 * 
 * Format: [num_points, 2] for [x, y]
 * Data type: float32
 * 
 * @param points Vector of 2D points in [x, y] format
 * @return Shared pointer to Holoscan tensor
 */
std::shared_ptr<holoscan::Tensor> create_points_tensor(
    const std::vector<std::vector<float>>& points);

/**
 * @brief Create a lines tensor for 2D lines
 * 
 * Format: [num_lines, 4] for [x1, y1, x2, y2]
 * Data type: float32
 * 
 * @param lines Vector of lines in [x1, y1, x2, y2] format
 * @return Shared pointer to Holoscan tensor
 */
std::shared_ptr<holoscan::Tensor> create_lines_tensor(
    const std::vector<std::vector<float>>& lines);

/**
 * @brief Create a triangles tensor for 2D triangles
 * 
 * Format: [num_triangles, 6] for [x1, y1, x2, y2, x3, y3]
 * Data type: float32
 * 
 * @param triangles Vector of triangles in [x1, y1, x2, y2, x3, y3] format
 * @return Shared pointer to Holoscan tensor
 */
std::shared_ptr<holoscan::Tensor> create_triangles_tensor(
    const std::vector<std::vector<float>>& triangles);

/**
 * @brief Validate tensor data for HolovizOp compatibility
 * 
 * @param tensor Holoscan tensor to validate
 * @param expected_type Expected tensor type ("rectangles", "text", "color_lut", etc.)
 * @return true if tensor is valid for the expected type, false otherwise
 */
bool validate_tensor_for_holoviz(const std::shared_ptr<holoscan::Tensor>& tensor,
                                const std::string& expected_type);

/**
 * @brief Get tensor shape as string for debugging
 * 
 * @param tensor Holoscan tensor
 * @return String representation of tensor shape
 */
std::string get_tensor_shape_string(const std::shared_ptr<holoscan::Tensor>& tensor);

/**
 * @brief Get tensor data type as string for debugging
 * 
 * @param tensor Holoscan tensor
 * @return String representation of tensor data type
 */
std::string get_tensor_dtype_string(const std::shared_ptr<holoscan::Tensor>& tensor);

/**
 * @brief Create a placeholder tensor for testing
 * 
 * @param tensor_type Type of tensor to create ("rectangles", "text", "color_lut")
 * @return Shared pointer to placeholder Holoscan tensor
 */
std::shared_ptr<holoscan::Tensor> create_placeholder_tensor(const std::string& tensor_type);

} // namespace holoviz_tensor_factory
} // namespace urology 