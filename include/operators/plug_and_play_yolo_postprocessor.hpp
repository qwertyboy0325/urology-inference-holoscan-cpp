#pragma once

#include <holoscan/holoscan.hpp>
#include "operators/plug_and_play_detection.hpp"
#include <vector>
#include <map>
#include <memory>
#include <string>

namespace urology {

/**
 * @brief Plug-and-Play YOLO Postprocessor Operator
 * 
 * This operator processes YOLO model outputs and emits a single, self-contained
 * tensor that enables zero-config visualization with HolovizOp. It eliminates
 * the need for custom InputSpec definitions and per-class configuration.
 * 
 * Output Format:
 * - Single detection tensor: [N, 16] with all detection information
 * - Optional masks tensor: [N, H, W] for segmentation masks
 * - Optional labels tensor: [N, L] for class labels
 * - Zero configuration required for HolovizOp integration
 */
class PlugAndPlayYoloPostprocessorOp : public holoscan::Operator {
public:
    HOLOSCAN_OPERATOR_FORWARD_ARGS(PlugAndPlayYoloPostprocessorOp)

    PlugAndPlayYoloPostprocessorOp();
    virtual ~PlugAndPlayYoloPostprocessorOp();

    void setup(holoscan::OperatorSpec& spec) override;
    void compute(holoscan::InputContext& op_input, 
                holoscan::OutputContext& op_output,
                holoscan::ExecutionContext& context) override;

    // Configuration methods
    void set_class_metadata(const std::vector<ClassMetadata>& classes);
    void set_confidence_threshold(float threshold);
    void set_nms_threshold(float threshold);
    void set_max_detections(int max_detections);
    void set_enable_masks(bool enable);
    void set_enable_labels(bool enable);

    // Getters
    float get_confidence_threshold() const { return confidence_threshold_; }
    float get_nms_threshold() const { return nms_threshold_; }
    int get_max_detections() const { return max_detections_; }
    bool get_enable_masks() const { return enable_masks_; }
    bool get_enable_labels() const { return enable_labels_; }
    const std::vector<ClassMetadata>& get_class_metadata() const { return class_metadata_; }

private:
    // Input processing
    void receive_inputs(holoscan::InputContext& op_input,
                       std::shared_ptr<holoscan::Tensor>& predictions,
                       std::shared_ptr<holoscan::Tensor>& masks);
    
    // Detection processing
    std::vector<Detection> process_detections(
        const std::shared_ptr<holoscan::Tensor>& predictions,
        const std::shared_ptr<holoscan::Tensor>& masks);
    
    std::vector<Detection> process_detections_gpu(
        const std::shared_ptr<holoscan::Tensor>& predictions,
        const std::shared_ptr<holoscan::Tensor>& masks);
    
    std::vector<Detection> process_detections_cpu(
        const std::shared_ptr<holoscan::Tensor>& predictions,
        const std::shared_ptr<holoscan::Tensor>& masks);
    
    // Output creation
    DetectionTensorMap create_plug_and_play_output(
        const std::vector<Detection>& detections);
    
    // Utility methods
    void update_detection_visualization(Detection& detection);
    void calculate_text_position(Detection& detection);
    void apply_color_scheme(Detection& detection);
    
    // Validation
    bool validate_inputs(const std::shared_ptr<holoscan::Tensor>& predictions,
                        const std::shared_ptr<holoscan::Tensor>& masks);
    
    // Error handling
    void handle_processing_error(const std::string& error_message);
    void emit_empty_output(holoscan::OutputContext& op_output);

    // Member variables
    std::vector<ClassMetadata> class_metadata_;
    float confidence_threshold_;
    float nms_threshold_;
    int max_detections_;
    bool enable_masks_;
    bool enable_labels_;
    int64_t frame_counter_;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point last_performance_log_;
    int64_t total_frames_processed_;
    double total_processing_time_ms_;
    
    // Default class metadata for urology use case
    void initialize_default_class_metadata();
};

} // namespace urology 