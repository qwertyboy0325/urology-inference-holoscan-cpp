#include "holoscan_fix.hpp"
#include "operators/holoviz_static_config.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace urology {

std::vector<holoscan::ops::HolovizOp::InputSpec> create_static_tensor_config(
    const LabelDict& label_dict) {
    
    std::vector<holoscan::ops::HolovizOp::InputSpec> config;
    
    // Add default color tensor for background
    config.push_back(holoscan::ops::HolovizOp::InputSpec("", "color"));
    
    // Add mask tensor for segmentation
    config.push_back(holoscan::ops::HolovizOp::InputSpec("masks", "color_lut"));
    
    // Add tensor specs for each class in the label dictionary
    for (const auto& [class_id, label_info] : label_dict) {
        std::string boxes_name = "boxes_" + std::to_string(class_id);
        std::string label_name = "labels_" + std::to_string(class_id);
        
        // Create bounding box tensor spec
        holoscan::ops::HolovizOp::InputSpec box_spec(boxes_name, "rectangles");
        box_spec.opacity_ = label_info.opacity;
        box_spec.color_ = label_info.color;
        config.push_back(box_spec);
        
        // Create label tensor spec
        holoscan::ops::HolovizOp::InputSpec label_spec(label_name, "text");
        label_spec.opacity_ = label_info.opacity;
        label_spec.color_ = label_info.color;
        config.push_back(label_spec);
    }
    
    std::cout << "Created static tensor configuration with "
              << config.size() << " tensor specs" << std::endl;
    
    return config;
}

std::vector<holoscan::ops::HolovizOp::InputSpec> create_minimal_static_config() {
    std::vector<holoscan::ops::HolovizOp::InputSpec> config;
    
    // Add minimal required tensors
    config.push_back(holoscan::ops::HolovizOp::InputSpec("", "color"));
    config.push_back(holoscan::ops::HolovizOp::InputSpec("masks", "color_lut"));
    
    std::cout << "Created minimal static configuration with "
              << config.size() << " tensor specs" << std::endl;
    
    return config;
}

std::vector<holoscan::ops::HolovizOp::InputSpec> create_static_config_for_classes(
    const LabelDict& label_dict,
    const std::vector<int>& class_ids) {
    
    std::vector<holoscan::ops::HolovizOp::InputSpec> config;
    
    // Add default tensors
    config.push_back(holoscan::ops::HolovizOp::InputSpec("", "color"));
    config.push_back(holoscan::ops::HolovizOp::InputSpec("masks", "color_lut"));
    
    // Add tensor specs only for specified classes
    for (int class_id : class_ids) {
        auto it = label_dict.find(class_id);
        if (it == label_dict.end()) {
            std::cerr << "Warning: Class ID " << class_id << " not found in label dictionary" << std::endl;
            continue;
        }
        
        const auto& label_info = it->second;
        std::string boxes_name = "boxes_" + std::to_string(class_id);
        std::string label_name = "labels_" + std::to_string(class_id);
        
        // Create bounding box tensor spec
        holoscan::ops::HolovizOp::InputSpec box_spec(boxes_name, "rectangles");
        box_spec.opacity_ = label_info.opacity;
        box_spec.color_ = label_info.color;
        config.push_back(box_spec);
        
        // Create label tensor spec
        holoscan::ops::HolovizOp::InputSpec label_spec(label_name, "text");
        label_spec.opacity_ = label_info.opacity;
        label_spec.color_ = label_info.color;
        config.push_back(label_spec);
    }
    
    std::cout << "Created static configuration for " << class_ids.size()
              << " classes with " << config.size() << " tensor specs" << std::endl;
    
    return config;
}

bool validate_static_config(const std::vector<holoscan::ops::HolovizOp::InputSpec>& config) {
    if (config.empty()) {
        std::cerr << "Static configuration is empty" << std::endl;
        return false;
    }
    
    // Check that all specs have valid tensor names and types
    for (const auto& spec : config) {
        if (spec.tensor_name_.empty() && spec.type_ != holoscan::ops::HolovizOp::InputType::COLOR) {
            std::cerr << "Invalid spec: empty tensor name for non-color type" << std::endl;
            return false;
        }
        
        // Validate type is one of the supported types
        switch (spec.type_) {
            case holoscan::ops::HolovizOp::InputType::COLOR:
            case holoscan::ops::HolovizOp::InputType::COLOR_LUT:
            case holoscan::ops::HolovizOp::InputType::RECTANGLES:
            case holoscan::ops::HolovizOp::InputType::TEXT:
                break;
            default:
                std::cerr << "Invalid spec type for tensor: " << spec.tensor_name_ << std::endl;
                return false;
        }
    }
    
    return true;
}

std::string get_config_summary(const std::vector<holoscan::ops::HolovizOp::InputSpec>& config) {
    std::ostringstream oss;
    oss << "Static Configuration Summary:\n";
    oss << "  Total tensor specs: " << config.size() << "\n";
    
    // Count by type
    std::map<holoscan::ops::HolovizOp::InputType, int> type_counts;
    for (const auto& spec : config) {
        type_counts[spec.type_]++;
    }
    
    oss << "  Type breakdown:\n";
    for (const auto& [type, count] : type_counts) {
        oss << "    - " << static_cast<int>(type) << ": " << count << "\n";
    }
    
    // List all specs
    oss << "  Tensor specs:\n";
    for (const auto& spec : config) {
        oss << "    - " << (spec.tensor_name_.empty() ? "(default)" : spec.tensor_name_)
            << " -> type " << static_cast<int>(spec.type_) << "\n";
    }
    
    return oss.str();
}

std::map<int, LabelInfo> create_default_urology_label_dict() {
    std::map<int, LabelInfo> label_dict;
    
    // Default urology class colors and names (matching Python implementation)
    // These are the standard colors used in the Python version
    std::vector<std::vector<float>> colors = {
        {1.0f, 0.0f, 0.0f, 1.0f},   // Red
        {0.0f, 1.0f, 0.0f, 1.0f},   // Green
        {0.0f, 0.0f, 1.0f, 1.0f},   // Blue
        {1.0f, 1.0f, 0.0f, 1.0f},   // Yellow
        {1.0f, 0.0f, 1.0f, 1.0f},   // Magenta
        {0.0f, 1.0f, 1.0f, 1.0f},   // Cyan
        {1.0f, 0.5f, 0.0f, 1.0f},   // Orange
        {0.5f, 0.0f, 1.0f, 1.0f},   // Purple
        {0.0f, 0.5f, 0.5f, 1.0f},   // Teal
        {0.5f, 0.5f, 0.0f, 1.0f}    // Olive
    };
    
    // Default urology class names
    std::vector<std::string> class_names = {
        "Kidney",
        "Ureter", 
        "Bladder",
        "Prostate",
        "Urethra",
        "Testis",
        "Penis",
        "Vagina",
        "Uterus",
        "Ovary"
    };
    
    // Create label dictionary entries
    for (size_t i = 0; i < class_names.size() && i < colors.size(); ++i) {
        int class_id = static_cast<int>(i);
        label_dict[class_id] = LabelInfo(
            class_names[i],           // text
            colors[i],                // color
            0.7f,                     // opacity
            4                         // line_width
        );
    }
    
    std::cout << "[STATIC_CONFIG] Created default urology label dictionary with " 
              << label_dict.size() << " classes" << std::endl;
    
    return label_dict;
}

std::vector<std::vector<float>> create_default_color_lut() {
    // Default color lookup table for segmentation masks
    // This matches the Python implementation's color scheme
    std::vector<std::vector<float>> color_lut = {
        {0.0f, 0.0f, 0.0f, 0.0f},   // Transparent (background)
        {1.0f, 0.0f, 0.0f, 0.7f},   // Red with transparency
        {0.0f, 1.0f, 0.0f, 0.7f},   // Green with transparency
        {0.0f, 0.0f, 1.0f, 0.7f},   // Blue with transparency
        {1.0f, 1.0f, 0.0f, 0.7f},   // Yellow with transparency
        {1.0f, 0.0f, 1.0f, 0.7f},   // Magenta with transparency
        {0.0f, 1.0f, 1.0f, 0.7f},   // Cyan with transparency
        {1.0f, 0.5f, 0.0f, 0.7f},   // Orange with transparency
        {0.5f, 0.0f, 1.0f, 0.7f},   // Purple with transparency
        {0.0f, 0.5f, 0.5f, 0.7f},   // Teal with transparency
        {0.5f, 0.5f, 0.0f, 0.7f}    // Olive with transparency
    };
    
    std::cout << "[STATIC_CONFIG] Created default color lookup table with " 
              << color_lut.size() << " colors" << std::endl;
    
    return color_lut;
}

} // namespace urology 