#pragma once

#include "operators/holoviz_common_types.hpp"
#include <holoscan/operators/holoviz/holoviz.hpp>
#include <vector>
#include <string>

namespace urology {

/**
 * @brief Create static tensor configuration for HolovizOp
 * 
 * This function creates a static tensor configuration that defines the expected
 * input tensors for HolovizOp. The configuration includes tensors for bounding boxes,
 * text labels, and image overlays.
 * 
 * @param label_dict Label dictionary mapping class IDs to label information
 * @return Vector of InputSpec objects defining the static tensor configuration
 */
std::vector<holoscan::ops::HolovizOp::InputSpec> create_static_tensor_config(
    const LabelDict& label_dict);

/**
 * @brief Create minimal static configuration for testing
 * 
 * Creates a minimal static configuration with just the essential tensors
 * for basic visualization testing.
 * 
 * @return Vector of InputSpec objects for minimal configuration
 */
std::vector<holoscan::ops::HolovizOp::InputSpec> create_minimal_static_config();

/**
 * @brief Create static configuration for specific classes
 * 
 * Creates a static configuration optimized for a specific set of classes,
 * with custom colors and styling for each class.
 * 
 * @param class_ids Vector of class IDs to include in configuration
 * @param label_dict Label dictionary for class information
 * @return Vector of InputSpec objects for the specified classes
 */
std::vector<holoscan::ops::HolovizOp::InputSpec> create_static_config_for_classes(
    const std::vector<int>& class_ids,
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

/**
 * @brief Get configuration summary
 * 
 * Generates a human-readable summary of the static configuration,
 * useful for debugging and documentation.
 * 
 * @param config Static configuration to summarize
 * @return String containing configuration summary
 */
std::string get_config_summary(const std::vector<holoscan::ops::HolovizOp::InputSpec>& config);

/**
 * @brief Create default urology label dictionary
 * 
 * Creates a default label dictionary with the standard urology classes
 * and their associated colors and styling.
 * 
 * @return LabelDict with default urology classes
 */
LabelDict create_default_urology_label_dict();

/**
 * @brief Create default color lookup table
 * 
 * Creates a default color lookup table with distinct colors for
 * visualization of different classes.
 * 
 * @return ColorLUT with default colors
 */
ColorLUT create_default_color_lut();

} // namespace urology 