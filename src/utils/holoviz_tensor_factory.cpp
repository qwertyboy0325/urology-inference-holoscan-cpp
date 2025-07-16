#include "holoscan_fix.hpp"
#include "operators/holoviz_tensor_factory.hpp"
#include "utils/yolo_utils.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>

namespace urology {
namespace holoviz_tensor_factory {

std::shared_ptr<holoscan::Tensor> create_rectangle_tensor(
    const std::vector<std::vector<float>>& boxes) {
    
    if (boxes.empty()) {
        std::cout << "[FACTORY] Warning: Empty boxes vector, creating placeholder tensor" << std::endl;
        return create_placeholder_tensor("rectangles");
    }
    
    // Validate box format
    for (const auto& box : boxes) {
        if (box.size() < 4) {
            std::cerr << "[FACTORY] Error: Box must have at least 4 coordinates [x1, y1, x2, y2]" << std::endl;
            return nullptr;
        }
    }
    
    // Flatten boxes into single vector
    std::vector<float> data;
    data.reserve(boxes.size() * 4);
    
    for (const auto& box : boxes) {
        // Ensure box has exactly 4 coordinates
        data.insert(data.end(), box.begin(), box.begin() + 4);
    }
    
    // Create tensor shape: [num_boxes, 4]
    std::vector<int64_t> shape = {static_cast<int64_t>(boxes.size()), 4};
    
    std::cout << "[FACTORY] Creating rectangle tensor with " << boxes.size() 
              << " boxes, shape: [" << shape[0] << ", " << shape[1] << "]" << std::endl;
    
    // Use the simple tensor creation function with float32
    auto tensor = urology::yolo_utils::make_holoscan_tensor_simple(data, shape);
    
    if (tensor) {
        std::cout << "[FACTORY] Rectangle tensor created successfully" << std::endl;
    } else {
        std::cerr << "[FACTORY] Error: Failed to create rectangle tensor" << std::endl;
    }
    
    return tensor;
}

std::shared_ptr<holoscan::Tensor> create_text_tensor(
    const std::vector<std::vector<float>>& positions) {
    
    if (positions.empty()) {
        std::cout << "[FACTORY] Warning: Empty positions vector, creating placeholder tensor" << std::endl;
        return create_placeholder_tensor("text");
    }
    
    // Validate position format
    for (const auto& pos : positions) {
        if (pos.size() < 3) {
            std::cerr << "[FACTORY] Error: Position must have at least 3 coordinates [x, y, size]" << std::endl;
            return nullptr;
        }
    }
    
    // Flatten positions into single vector
    std::vector<float> data;
    data.reserve(positions.size() * 3);
    
    for (const auto& pos : positions) {
        // Ensure position has exactly 3 coordinates
        data.insert(data.end(), pos.begin(), pos.begin() + 3);
    }
    
    // Create tensor shape: [num_texts, 3]
    std::vector<int64_t> shape = {static_cast<int64_t>(positions.size()), 3};
    
    std::cout << "[FACTORY] Creating text tensor with " << positions.size() 
              << " positions, shape: [" << shape[0] << ", " << shape[1] << "]" << std::endl;
    
    // Use the simple tensor creation function with float32
    auto tensor = urology::yolo_utils::make_holoscan_tensor_simple(data, shape);
    
    if (tensor) {
        std::cout << "[FACTORY] Text tensor created successfully" << std::endl;
    } else {
        std::cerr << "[FACTORY] Error: Failed to create text tensor" << std::endl;
    }
    
    return tensor;
}

std::shared_ptr<holoscan::Tensor> create_color_lut_tensor(
    const std::vector<std::vector<float>>& masks,
    int height, int width) {
    
    if (masks.empty()) {
        std::cout << "[FACTORY] Warning: Empty masks vector, creating placeholder tensor" << std::endl;
        return create_placeholder_tensor("color_lut");
    }
    
    if (height <= 0 || width <= 0) {
        std::cerr << "[FACTORY] Error: Invalid dimensions: height=" << height << ", width=" << width << std::endl;
        return nullptr;
    }
    
    // Validate mask dimensions
    size_t expected_size = height * width;
    for (const auto& mask : masks) {
        if (mask.size() != expected_size) {
            std::cerr << "[FACTORY] Error: Mask size mismatch. Expected " << expected_size 
                      << ", got " << mask.size() << std::endl;
            return nullptr;
        }
    }
    
    // Flatten masks into single vector
    std::vector<float> data;
    data.reserve(masks.size() * expected_size);
    
    for (const auto& mask : masks) {
        data.insert(data.end(), mask.begin(), mask.end());
    }
    
    // Create tensor shape: [num_masks, height, width]
    std::vector<int64_t> shape = {static_cast<int64_t>(masks.size()), 
                                  static_cast<int64_t>(height), 
                                  static_cast<int64_t>(width)};
    
    std::cout << "[FACTORY] Creating color LUT tensor with " << masks.size() 
              << " masks, shape: [" << shape[0] << ", " << shape[1] << ", " << shape[2] << "]" << std::endl;
    
    // Use the simple tensor creation function with float32
    auto tensor = urology::yolo_utils::make_holoscan_tensor_simple(data, shape);
    
    if (tensor) {
        std::cout << "[FACTORY] Color LUT tensor created successfully" << std::endl;
    } else {
        std::cerr << "[FACTORY] Error: Failed to create color LUT tensor" << std::endl;
    }
    
    return tensor;
}

std::shared_ptr<holoscan::Tensor> create_color_tensor(
    const std::vector<uint8_t>& data,
    int height, int width, int channels,
    const std::string& data_type,
    const std::string& layout) {
    
    if (data.empty()) {
        std::cerr << "[FACTORY] Error: Empty image data" << std::endl;
        return nullptr;
    }
    
    if (height <= 0 || width <= 0 || channels <= 0) {
        std::cerr << "[FACTORY] Error: Invalid dimensions: height=" << height 
                  << ", width=" << width << ", channels=" << channels << std::endl;
        return nullptr;
    }
    
    size_t expected_size = height * width * channels;
    if (data.size() != expected_size) {
        std::cerr << "[FACTORY] Error: Data size mismatch. Expected " << expected_size 
                  << ", got " << data.size() << std::endl;
        return nullptr;
    }
    
    // Create tensor shape based on layout
    std::vector<int64_t> shape;
    if (layout == "hwc") {
        shape = {static_cast<int64_t>(height), static_cast<int64_t>(width), static_cast<int64_t>(channels)};
    } else if (layout == "chw") {
        shape = {static_cast<int64_t>(channels), static_cast<int64_t>(height), static_cast<int64_t>(width)};
    } else {
        std::cerr << "[FACTORY] Error: Unsupported layout: " << layout << std::endl;
        return nullptr;
    }
    
    std::cout << "[FACTORY] Creating color tensor with shape: [";
    for (size_t i = 0; i < shape.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << shape[i];
    }
    std::cout << "], data_type: " << data_type << ", layout: " << layout << std::endl;
    
    // Convert data type if needed
    std::shared_ptr<holoscan::Tensor> tensor;
    if (data_type == "uint8") {
        // Create tensor with uint8 data
        tensor = urology::yolo_utils::make_holoscan_tensor_from_data(
            const_cast<void*>(static_cast<const void*>(data.data())), 
            shape, 0, 8, 1); // kDLUInt=0, 8 bits, 1 lane
    } else if (data_type == "float32") {
        // Convert uint8 to float32
        std::vector<float> float_data(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            float_data[i] = static_cast<float>(data[i]) / 255.0f;
        }
        tensor = urology::yolo_utils::make_holoscan_tensor_simple(float_data, shape);
    } else {
        std::cerr << "[FACTORY] Error: Unsupported data type: " << data_type << std::endl;
        return nullptr;
    }
    
    if (tensor) {
        std::cout << "[FACTORY] Color tensor created successfully" << std::endl;
    } else {
        std::cerr << "[FACTORY] Error: Failed to create color tensor" << std::endl;
    }
    
    return tensor;
}

std::shared_ptr<holoscan::Tensor> create_points_tensor(
    const std::vector<std::vector<float>>& points) {
    
    if (points.empty()) {
        std::cout << "[FACTORY] Warning: Empty points vector, creating placeholder tensor" << std::endl;
        return create_placeholder_tensor("points");
    }
    
    // Validate point format
    for (const auto& point : points) {
        if (point.size() < 2) {
            std::cerr << "[FACTORY] Error: Point must have at least 2 coordinates [x, y]" << std::endl;
            return nullptr;
        }
    }
    
    // Flatten points into single vector
    std::vector<float> data;
    data.reserve(points.size() * 2);
    
    for (const auto& point : points) {
        // Ensure point has exactly 2 coordinates
        data.insert(data.end(), point.begin(), point.begin() + 2);
    }
    
    // Create tensor shape: [num_points, 2]
    std::vector<int64_t> shape = {static_cast<int64_t>(points.size()), 2};
    
    std::cout << "[FACTORY] Creating points tensor with " << points.size() 
              << " points, shape: [" << shape[0] << ", " << shape[1] << "]" << std::endl;
    
    auto tensor = urology::yolo_utils::make_holoscan_tensor_simple(data, shape);
    
    if (tensor) {
        std::cout << "[FACTORY] Points tensor created successfully" << std::endl;
    } else {
        std::cerr << "[FACTORY] Error: Failed to create points tensor" << std::endl;
    }
    
    return tensor;
}

std::shared_ptr<holoscan::Tensor> create_lines_tensor(
    const std::vector<std::vector<float>>& lines) {
    
    if (lines.empty()) {
        std::cout << "[FACTORY] Warning: Empty lines vector, creating placeholder tensor" << std::endl;
        return create_placeholder_tensor("lines");
    }
    
    // Validate line format
    for (const auto& line : lines) {
        if (line.size() < 4) {
            std::cerr << "[FACTORY] Error: Line must have at least 4 coordinates [x1, y1, x2, y2]" << std::endl;
            return nullptr;
        }
    }
    
    // Flatten lines into single vector
    std::vector<float> data;
    data.reserve(lines.size() * 4);
    
    for (const auto& line : lines) {
        // Ensure line has exactly 4 coordinates
        data.insert(data.end(), line.begin(), line.begin() + 4);
    }
    
    // Create tensor shape: [num_lines, 4]
    std::vector<int64_t> shape = {static_cast<int64_t>(lines.size()), 4};
    
    std::cout << "[FACTORY] Creating lines tensor with " << lines.size() 
              << " lines, shape: [" << shape[0] << ", " << shape[1] << "]" << std::endl;
    
    auto tensor = urology::yolo_utils::make_holoscan_tensor_simple(data, shape);
    
    if (tensor) {
        std::cout << "[FACTORY] Lines tensor created successfully" << std::endl;
    } else {
        std::cerr << "[FACTORY] Error: Failed to create lines tensor" << std::endl;
    }
    
    return tensor;
}

std::shared_ptr<holoscan::Tensor> create_triangles_tensor(
    const std::vector<std::vector<float>>& triangles) {
    
    if (triangles.empty()) {
        std::cout << "[FACTORY] Warning: Empty triangles vector, creating placeholder tensor" << std::endl;
        return create_placeholder_tensor("triangles");
    }
    
    // Validate triangle format
    for (const auto& triangle : triangles) {
        if (triangle.size() < 6) {
            std::cerr << "[FACTORY] Error: Triangle must have at least 6 coordinates [x1, y1, x2, y2, x3, y3]" << std::endl;
            return nullptr;
        }
    }
    
    // Flatten triangles into single vector
    std::vector<float> data;
    data.reserve(triangles.size() * 6);
    
    for (const auto& triangle : triangles) {
        // Ensure triangle has exactly 6 coordinates
        data.insert(data.end(), triangle.begin(), triangle.begin() + 6);
    }
    
    // Create tensor shape: [num_triangles, 6]
    std::vector<int64_t> shape = {static_cast<int64_t>(triangles.size()), 6};
    
    std::cout << "[FACTORY] Creating triangles tensor with " << triangles.size() 
              << " triangles, shape: [" << shape[0] << ", " << shape[1] << "]" << std::endl;
    
    auto tensor = urology::yolo_utils::make_holoscan_tensor_simple(data, shape);
    
    if (tensor) {
        std::cout << "[FACTORY] Triangles tensor created successfully" << std::endl;
    } else {
        std::cerr << "[FACTORY] Error: Failed to create triangles tensor" << std::endl;
    }
    
    return tensor;
}

bool validate_tensor_for_holoviz(const std::shared_ptr<holoscan::Tensor>& tensor,
                                const std::string& expected_type) {
    
    if (!tensor) {
        std::cerr << "[VALIDATION] Error: Tensor is null" << std::endl;
        return false;
    }
    
    auto shape = tensor->shape();
    if (shape.empty()) {
        std::cerr << "[VALIDATION] Error: Tensor has empty shape" << std::endl;
        return false;
    }
    
    // Basic validation based on expected type
    if (expected_type == "rectangles") {
        if (shape.size() != 2 || shape[1] != 4) {
            std::cerr << "[VALIDATION] Error: Rectangle tensor should have shape [num_boxes, 4], got [";
            for (size_t i = 0; i < shape.size(); ++i) {
                if (i > 0) std::cerr << ", ";
                std::cerr << shape[i];
            }
            std::cerr << "]" << std::endl;
            return false;
        }
    } else if (expected_type == "text") {
        if (shape.size() != 2 || shape[1] != 3) {
            std::cerr << "[VALIDATION] Error: Text tensor should have shape [num_texts, 3], got [";
            for (size_t i = 0; i < shape.size(); ++i) {
                if (i > 0) std::cerr << ", ";
                std::cerr << shape[i];
            }
            std::cerr << "]" << std::endl;
            return false;
        }
    } else if (expected_type == "color_lut") {
        if (shape.size() != 3) {
            std::cerr << "[VALIDATION] Error: Color LUT tensor should have shape [num_masks, height, width], got [";
            for (size_t i = 0; i < shape.size(); ++i) {
                if (i > 0) std::cerr << ", ";
                std::cerr << shape[i];
            }
            std::cerr << "]" << std::endl;
            return false;
        }
    }
    
    std::cout << "[VALIDATION] Tensor validation passed for type: " << expected_type << std::endl;
    return true;
}

std::string get_tensor_shape_string(const std::shared_ptr<holoscan::Tensor>& tensor) {
    if (!tensor) {
        return "null";
    }
    
    auto shape = tensor->shape();
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < shape.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << shape[i];
    }
    oss << "]";
    return oss.str();
}

std::string get_tensor_dtype_string(const std::shared_ptr<holoscan::Tensor>& tensor) {
    if (!tensor) {
        return "null";
    }
    
    // This is a simplified version - in a real implementation, you'd need to
    // access the actual DLPack dtype information
    return "float32"; // Assuming float32 for now
}

std::shared_ptr<holoscan::Tensor> create_placeholder_tensor(const std::string& tensor_type) {
    std::cout << "[FACTORY] Creating placeholder tensor for type: " << tensor_type << std::endl;
    
    if (tensor_type == "rectangles") {
        // Create a single empty rectangle
        std::vector<float> data = {0.0f, 0.0f, 0.1f, 0.1f};
        std::vector<int64_t> shape = {1, 4};
        return urology::yolo_utils::make_holoscan_tensor_simple(data, shape);
    } else if (tensor_type == "text") {
        // Create a single text position
        std::vector<float> data = {0.0f, 0.0f, 0.04f};
        std::vector<int64_t> shape = {1, 3};
        return urology::yolo_utils::make_holoscan_tensor_simple(data, shape);
    } else if (tensor_type == "color_lut") {
        // Create a single 1x1 mask
        std::vector<float> data = {0.0f};
        std::vector<int64_t> shape = {1, 1, 1};
        return urology::yolo_utils::make_holoscan_tensor_simple(data, shape);
    } else if (tensor_type == "points") {
        // Create a single point
        std::vector<float> data = {0.0f, 0.0f};
        std::vector<int64_t> shape = {1, 2};
        return urology::yolo_utils::make_holoscan_tensor_simple(data, shape);
    } else if (tensor_type == "lines") {
        // Create a single line
        std::vector<float> data = {0.0f, 0.0f, 0.1f, 0.1f};
        std::vector<int64_t> shape = {1, 4};
        return urology::yolo_utils::make_holoscan_tensor_simple(data, shape);
    } else if (tensor_type == "triangles") {
        // Create a single triangle
        std::vector<float> data = {0.0f, 0.0f, 0.1f, 0.0f, 0.05f, 0.1f};
        std::vector<int64_t> shape = {1, 6};
        return urology::yolo_utils::make_holoscan_tensor_simple(data, shape);
    } else {
        std::cerr << "[FACTORY] Error: Unknown tensor type for placeholder: " << tensor_type << std::endl;
        return nullptr;
    }
}

} // namespace holoviz_tensor_factory
} // namespace urology 