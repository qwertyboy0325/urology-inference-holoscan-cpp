#include "operators/holoviz_compatible_tensors.hpp"
#include "utils/yolo_utils.hpp"
#include <sstream>
#include <iostream>

namespace urology {

void HolovizCompatibleTensors::add_rectangle_tensor(const std::string& name,
                                                   const std::vector<std::vector<float>>& boxes) {
    // Convert boxes to tensor format
    std::vector<float> flat_boxes;
    for (const auto& box : boxes) {
        flat_boxes.insert(flat_boxes.end(), box.begin(), box.end());
    }
    
    // Create tensor with shape [num_boxes, 4] for [x1, y1, x2, y2]
    std::vector<int64_t> shape = {static_cast<int64_t>(boxes.size()), 4};
    auto tensor = urology::yolo_utils::make_holoscan_tensor_simple(flat_boxes, shape);
    tensors_[name] = tensor;
}

void HolovizCompatibleTensors::add_text_tensor(const std::string& name,
                                              const std::vector<std::vector<float>>& positions) {
    // Convert positions to tensor format
    std::vector<float> flat_positions;
    for (const auto& pos : positions) {
        flat_positions.insert(flat_positions.end(), pos.begin(), pos.end());
    }
    
    // Create tensor with shape [num_positions, 3] for [x, y, scale]
    std::vector<int64_t> shape = {static_cast<int64_t>(positions.size()), 3};
    auto tensor = urology::yolo_utils::make_holoscan_tensor_simple(flat_positions, shape);
    tensors_[name] = tensor;
}

void HolovizCompatibleTensors::add_image_tensor(const std::string& name,
                                               const std::shared_ptr<holoscan::Tensor>& tensor) {
    tensors_[name] = tensor;
}

void HolovizCompatibleTensors::add_mask_tensor(const std::string& name,
                                              const std::shared_ptr<holoscan::Tensor>& tensor) {
    tensors_[name] = tensor;
}

std::shared_ptr<holoscan::Tensor> HolovizCompatibleTensors::get_tensor(const std::string& name) const {
    auto it = tensors_.find(name);
    return (it != tensors_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<holoscan::Tensor>> HolovizCompatibleTensors::get_all_tensors() const {
    std::vector<std::shared_ptr<holoscan::Tensor>> result;
    result.reserve(tensors_.size());
    for (const auto& [name, tensor] : tensors_) {
        result.push_back(tensor);
    }
    return result;
}

std::vector<holoscan::ops::HolovizOp::InputSpec> HolovizCompatibleTensors::generate_input_specs() const {
    std::vector<holoscan::ops::HolovizOp::InputSpec> specs;
    specs.reserve(tensors_.size());
    
    for (const auto& [name, tensor] : tensors_) {
        // Determine tensor type based on name or shape
        if (name.find("box") != std::string::npos || name.find("rect") != std::string::npos) {
            specs.push_back(create_rectangle_spec(name));
        } else if (name.find("text") != std::string::npos || name.find("label") != std::string::npos) {
            specs.push_back(create_text_spec(name));
        } else if (name.find("mask") != std::string::npos) {
            specs.push_back(create_mask_spec(name));
        } else {
            specs.push_back(create_image_spec(name));
        }
    }
    
    return specs;
}

std::vector<holoscan::ops::HolovizOp::InputSpec> HolovizCompatibleTensors::generate_dynamic_input_specs(
    const std::vector<Detection>& detections) const {
    return urology::create_dynamic_input_specs(detections, label_dict);
}

bool HolovizCompatibleTensors::validate() const {
    // Check that all tensors are valid
    for (const auto& [name, tensor] : tensors_) {
        if (!tensor) {
            std::cerr << "Invalid tensor: " << name << std::endl;
            return false;
        }
    }
    
    // Check that label dictionary is not empty if we have tensors
    if (!tensors_.empty() && label_dict.empty()) {
        std::cerr << "Label dictionary is empty but tensors exist" << std::endl;
        return false;
    }
    
    return true;
}

std::string HolovizCompatibleTensors::get_summary() const {
    std::ostringstream oss;
    oss << "HolovizCompatibleTensors Summary:\n";
    oss << "  Total tensors: " << tensors_.size() << "\n";
    oss << "  Label dictionary size: " << label_dict.size() << "\n";
    oss << "  Color LUT size: " << color_lut.size() << "\n";
    
    for (const auto& [name, tensor] : tensors_) {
        oss << "  - " << name << ": ";
        if (tensor) {
            auto shape = tensor->shape();
            oss << "shape [";
            for (size_t i = 0; i < shape.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << shape[i];
            }
            oss << "]";
        } else {
            oss << "null";
        }
        oss << "\n";
    }
    
    return oss.str();
}

void HolovizCompatibleTensors::clear() {
    tensors_.clear();
}

void HolovizCompatibleTensors::reserve(size_t count) {
    tensors_.reserve(count);
}

bool HolovizCompatibleTensors::has_tensor(const std::string& name) const {
    return tensors_.find(name) != tensors_.end();
}

holoscan::ops::HolovizOp::InputSpec HolovizCompatibleTensors::create_rectangle_spec(const std::string& name) const {
    holoscan::ops::HolovizOp::InputSpec spec;
    spec.tensor_name_ = name;
    spec.type_ = holoscan::ops::HolovizOp::InputType::RECTANGLES;
    return spec;
}

holoscan::ops::HolovizOp::InputSpec HolovizCompatibleTensors::create_text_spec(const std::string& name) const {
    holoscan::ops::HolovizOp::InputSpec spec;
    spec.tensor_name_ = name;
    spec.type_ = holoscan::ops::HolovizOp::InputType::TEXT;
    return spec;
}

holoscan::ops::HolovizOp::InputSpec HolovizCompatibleTensors::create_image_spec(const std::string& name) const {
    holoscan::ops::HolovizOp::InputSpec spec;
    spec.tensor_name_ = name;
    spec.type_ = holoscan::ops::HolovizOp::InputType::COLOR;
    return spec;
}

holoscan::ops::HolovizOp::InputSpec HolovizCompatibleTensors::create_mask_spec(const std::string& name) const {
    holoscan::ops::HolovizOp::InputSpec spec;
    spec.tensor_name_ = name;
    spec.type_ = holoscan::ops::HolovizOp::InputType::COLOR_LUT;
    return spec;
}

} // namespace urology 