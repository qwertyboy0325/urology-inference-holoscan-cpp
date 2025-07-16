#pragma once

#include "holoscan_fix.hpp"
#include "operators/holoviz_compatible_tensors.hpp"
#include "operators/holoviz_static_config.hpp"
#include "operators/holoviz_common_types.hpp"
#include "utils/performance_monitor.hpp"
#include "utils/logger.hpp"
#include "utils/yolo_utils.hpp"

#include <holoscan/holoscan.hpp>
#include <holoscan/core/operator.hpp>
#include <holoscan/core/parameter.hpp>
#include <holoscan/core/operator_spec.hpp>
#include <holoscan/core/io_context.hpp>
#include <holoscan/core/execution_context.hpp>
#include <holoscan/operators/holoviz/holoviz.hpp>

#include <memory>
#include <vector>
#include <map>
#include <string>

namespace urology {

/**
 * @brief HolovizOp-native YOLO postprocessor that generates tensors compatible with HolovizOp
 * 
 * This operator processes YOLO inference results and generates tensors that can be directly
 * consumed by HolovizOp without requiring custom InputSpec definitions. It combines static
 * tensor configuration with dynamic InputSpec generation for optimal performance and flexibility.
 */
class HolovizNativeYoloPostprocessor : public holoscan::Operator {
public:
    HOLOSCAN_OPERATOR_FORWARD_ARGS(HolovizNativeYoloPostprocessor)
    
    HolovizNativeYoloPostprocessor() = default;
    
    void setup(holoscan::OperatorSpec& spec) override;
    void initialize() override;
    void compute(holoscan::InputContext& op_input, 
                 holoscan::OutputContext& op_output, 
                 holoscan::ExecutionContext& context) override;

private:
    // Parameters
    holoscan::Parameter<std::vector<float>> confidence_threshold_;
    holoscan::Parameter<std::vector<float>> nms_threshold_;
    holoscan::Parameter<LabelDict> label_dict_;
    holoscan::Parameter<ColorLUT> color_lut_;
    holoscan::Parameter<int> input_width_;
    holoscan::Parameter<int> input_height_;
    holoscan::Parameter<bool> enable_gpu_processing_;
    holoscan::Parameter<bool> enable_debug_output_;
    
    // Cached parameters
    std::vector<float> confidence_threshold_cache_;
    std::vector<float> nms_threshold_cache_;
    LabelDict label_dict_cache_;
    ColorLUT color_lut_cache_;
    int input_width_cache_;
    int input_height_cache_;
    bool enable_gpu_processing_cache_;
    bool enable_debug_output_cache_;
    
    // Static tensor configuration
    std::vector<holoscan::ops::HolovizOp::InputSpec> static_tensor_config_;
    
    // Processing methods
    std::vector<Detection> process_input_tensor(const holoscan::Tensor& input_tensor);
    std::vector<Detection> extract_detections(const holoscan::Tensor& outputs_tensor, 
                                             const holoscan::Tensor& proto_tensor);
    std::vector<Detection> filter_detections(const std::vector<Detection>& detections);
    std::vector<Detection> apply_nms(const std::vector<Detection>& detections, float nms_threshold);
    void normalize_coordinates(std::vector<Detection>& detections);
    
    // Output generation
    void generate_holoviz_tensors(const std::vector<Detection>& detections,
                                 holoscan::OutputContext& op_output);
    void generate_bounding_box_tensors(const std::vector<Detection>& detections,
                                      HolovizCompatibleTensors& compatible_tensors);
    void generate_text_tensors(const std::vector<Detection>& detections,
                              HolovizCompatibleTensors& compatible_tensors);
    void generate_mask_tensors(const std::vector<Detection>& detections,
                              HolovizCompatibleTensors& compatible_tensors);
    
    // Dynamic InputSpec generation
    std::vector<holoscan::ops::HolovizOp::InputSpec> generate_dynamic_input_specs(
        const std::vector<Detection>& detections);
    
    // Utility methods
    void update_caches();
    void initialize_static_config();
    void log_detection_summary(const std::vector<Detection>& detections);
    void validate_parameters();
    
    // Constants
    static constexpr int kMaxDetections = 100;
    static constexpr float kDefaultConfidenceThreshold = 0.5f;
    static constexpr float kDefaultNmsThreshold = 0.45f;
    static constexpr int kDefaultInputWidth = 640;
    static constexpr int kDefaultInputHeight = 640;
};

} // namespace urology 