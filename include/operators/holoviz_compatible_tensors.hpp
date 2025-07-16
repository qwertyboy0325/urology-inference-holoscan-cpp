#pragma once

#include "operators/holoviz_common_types.hpp"
#include <holoscan/operators/holoviz/holoviz.hpp>
#include <holoscan/core/domain/tensor.hpp>
#include <memory>
#include <vector>
#include <string>
#include <map>

namespace urology {

/**
 * @brief HolovizOp-compatible tensor management class
 * 
 * This class manages multiple tensors that are compatible with HolovizOp,
 * providing a unified interface for creating, storing, and accessing
 * visualization tensors. It supports both static and dynamic tensor
 * configurations.
 */
class HolovizCompatibleTensors {
public:
    HolovizCompatibleTensors() = default;
    ~HolovizCompatibleTensors() = default;
    
    // Label and color management
    LabelDict label_dict;
    ColorLUT color_lut;
    
    // Tensor management
    void add_rectangle_tensor(const std::string& name, 
                             const std::vector<std::vector<float>>& boxes);
    void add_text_tensor(const std::string& name, 
                        const std::vector<std::vector<float>>& positions);
    void add_image_tensor(const std::string& name, 
                         const std::shared_ptr<holoscan::Tensor>& tensor);
    void add_mask_tensor(const std::string& name, 
                        const std::shared_ptr<holoscan::Tensor>& tensor);
    
    // Tensor access
    std::shared_ptr<holoscan::Tensor> get_tensor(const std::string& name) const;
    std::vector<std::shared_ptr<holoscan::Tensor>> get_all_tensors() const;
    size_t tensor_count() const { return tensors_.size(); }
    
    // InputSpec generation
    std::vector<holoscan::ops::HolovizOp::InputSpec> generate_input_specs() const;
    std::vector<holoscan::ops::HolovizOp::InputSpec> generate_dynamic_input_specs(
        const std::vector<Detection>& detections) const;
    
    // Validation
    bool validate() const;
    std::string get_summary() const;
    
    // Utility methods
    void clear();
    void reserve(size_t count);
    bool has_tensor(const std::string& name) const;
    
private:
    std::map<std::string, std::shared_ptr<holoscan::Tensor>> tensors_;
    
    // Helper methods
    holoscan::ops::HolovizOp::InputSpec create_rectangle_spec(const std::string& name) const;
    holoscan::ops::HolovizOp::InputSpec create_text_spec(const std::string& name) const;
    holoscan::ops::HolovizOp::InputSpec create_image_spec(const std::string& name) const;
    holoscan::ops::HolovizOp::InputSpec create_mask_spec(const std::string& name) const;
};

/**
 * @brief Create dynamic InputSpecs for detections
 * 
 * Generates dynamic InputSpecs based on the detected objects,
 * creating appropriate visualization elements for each detection.
 * 
 * @param detections Vector of detections to visualize
 * @param label_dict Label dictionary for class information
 * @return Vector of InputSpec objects for the detections
 */
std::vector<holoscan::ops::HolovizOp::InputSpec> create_dynamic_input_specs(
    const std::vector<Detection>& detections,
    const LabelDict& label_dict);

/**
 * @brief Create static tensor configuration
 * 
 * Creates a static tensor configuration that defines the expected
 * input tensors for HolovizOp based on the label dictionary.
 * 
 * @param label_dict Label dictionary mapping class IDs to label information
 * @return Vector of InputSpec objects defining the static tensor configuration
 */
std::vector<holoscan::ops::HolovizOp::InputSpec> create_static_tensor_config(
    const LabelDict& label_dict);

/**
 * @brief Validate static configuration
 * 
 * Validates that a static configuration is properly formed and contains
 * all required tensor specifications.
 * 
 * @param config Static configuration to validate
 * @return True if configuration is valid, false otherwise
 */
bool validate_static_config(const std::vector<holoscan::ops::HolovizOp::InputSpec>& config);

} // namespace urology 