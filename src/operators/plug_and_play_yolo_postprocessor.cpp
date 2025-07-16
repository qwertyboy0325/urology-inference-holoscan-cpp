#include "holoscan_fix.hpp"
#include "operators/plug_and_play_yolo_postprocessor.hpp"
#include "utils/logger.hpp"
#include "utils/plug_and_play_tensor_factory.hpp"
#include <chrono>
#include <algorithm>
#include <numeric>

namespace urology {

// Constructor
PlugAndPlayYoloPostprocessorOp::PlugAndPlayYoloPostprocessorOp()
    : confidence_threshold_(0.2f),
      nms_threshold_(0.45f),
      max_detections_(100),
      enable_masks_(true),
      enable_labels_(true),
      frame_counter_(0),
      total_frames_processed_(0),
      total_processing_time_ms_(0.0) {
    
    logger::info("Creating PlugAndPlayYoloPostprocessorOp");
    last_performance_log_ = std::chrono::high_resolution_clock::now();
    
    // Initialize default class metadata for urology use case
    initialize_default_class_metadata();
}

// Destructor
PlugAndPlayYoloPostprocessorOp::~PlugAndPlayYoloPostprocessorOp() {
    logger::info("Destroying PlugAndPlayYoloPostprocessorOp");
    
    // Log final performance statistics
    if (total_frames_processed_ > 0) {
        double avg_processing_time = total_processing_time_ms_ / total_frames_processed_;
        logger::info("Final performance stats: {} frames, {:.2f}ms avg processing time", 
                     total_frames_processed_, avg_processing_time);
    }
}

// Setup method
void PlugAndPlayYoloPostprocessorOp::setup(holoscan::OperatorSpec& spec) {
    logger::info("Setting up PlugAndPlayYoloPostprocessorOp");
    
    // Input specification
    spec.input("in");
    
    // Output specification - single output for plug-and-play format
    spec.output("out");
    
    // Parameters
    spec.param(confidence_threshold_, "confidence_threshold", "Confidence threshold for detections");
    spec.param(nms_threshold_, "nms_threshold", "NMS IoU threshold");
    spec.param(max_detections_, "max_detections", "Maximum number of detections to output");
    spec.param(enable_masks_, "enable_masks", "Enable segmentation mask output");
    spec.param(enable_labels_, "enable_labels", "Enable label text output");
    
    logger::info("PlugAndPlayYoloPostprocessorOp setup completed");
    logger::info("Parameters: confidence_threshold={}, nms_threshold={}, max_detections={}, enable_masks={}, enable_labels={}",
                 confidence_threshold_, nms_threshold_, max_detections_, enable_masks_, enable_labels_);
}

// Compute method
void PlugAndPlayYoloPostprocessorOp::compute(holoscan::InputContext& op_input, 
                                            holoscan::OutputContext& op_output,
                                            holoscan::ExecutionContext& context) {
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        logger::debug("Processing frame {}", frame_counter_);
        
        // Step 1: Receive inputs
        std::shared_ptr<holoscan::Tensor> predictions;
        std::shared_ptr<holoscan::Tensor> masks;
        receive_inputs(op_input, predictions, masks);
        
        // Step 2: Validate inputs
        if (!validate_inputs(predictions, masks)) {
            handle_processing_error("Input validation failed");
            emit_empty_output(op_output);
            return;
        }
        
        // Step 3: Process detections
        std::vector<Detection> detections = process_detections(predictions, masks);
        
        // Step 4: Create plug-and-play output
        DetectionTensorMap output = create_plug_and_play_output(detections);
        
        // Step 5: Validate output
        if (!output.is_valid()) {
            std::string validation_error = output.validate();
            handle_processing_error("Output validation failed: " + validation_error);
            emit_empty_output(op_output);
            return;
        }
        
        // Step 6: Emit output
        op_output.emit(output, "out");
        
        // Step 7: Update performance tracking
        auto end_time = std::chrono::high_resolution_clock::now();
        auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        double processing_time_ms = processing_time.count() / 1000.0;
        
        total_frames_processed_++;
        total_processing_time_ms_ += processing_time_ms;
        
        // Log performance every 100 frames
        if (frame_counter_ % 100 == 0) {
            double avg_processing_time = total_processing_time_ms_ / total_frames_processed_;
            logger::info("Performance stats: frame {}, {} detections, {:.2f}ms processing time, {:.2f}ms avg",
                         frame_counter_, detections.size(), processing_time_ms, avg_processing_time);
        }
        
        frame_counter_++;
        
        logger::debug("Successfully processed frame {} with {} detections", 
                     frame_counter_ - 1, detections.size());
        
    } catch (const std::exception& e) {
        handle_processing_error("Exception in compute: " + std::string(e.what()));
        emit_empty_output(op_output);
    }
}

// Input processing
void PlugAndPlayYoloPostprocessorOp::receive_inputs(holoscan::InputContext& op_input,
                                                   std::shared_ptr<holoscan::Tensor>& predictions,
                                                   std::shared_ptr<holoscan::Tensor>& masks) {
    
    auto in_message = op_input.receive<holoscan::TensorMap>("in");
    if (!in_message) {
        throw std::runtime_error("Failed to receive input message");
    }
    
    // Debug: Print available tensor names
    logger::debug("Available tensor names in input message:");
    for (const auto& [name, tensor] : *in_message) {
        logger::debug("  - {}", name);
    }
    
    // Extract predictions tensor
    if (in_message->find("outputs") != in_message->end()) {
        predictions = (*in_message)["outputs"];
        logger::debug("Found 'outputs' tensor for predictions");
    } else if (in_message->find("input_0") != in_message->end()) {
        predictions = (*in_message)["input_0"];
        logger::debug("Found 'input_0' tensor for predictions");
    } else {
        throw std::runtime_error("Missing predictions tensor (expected 'outputs' or 'input_0')");
    }
    
    // Extract masks tensor (optional)
    if (in_message->find("proto") != in_message->end()) {
        masks = (*in_message)["proto"];
        logger::debug("Found 'proto' tensor for masks");
    } else if (in_message->find("input_1") != in_message->end()) {
        masks = (*in_message)["input_1"];
        logger::debug("Found 'input_1' tensor for masks");
    } else {
        logger::debug("No masks tensor found (this is optional)");
        masks = nullptr;
    }
}

// Detection processing (main method)
std::vector<Detection> PlugAndPlayYoloPostprocessorOp::process_detections(
    const std::shared_ptr<holoscan::Tensor>& predictions,
    const std::shared_ptr<holoscan::Tensor>& masks) {
    
    // Try GPU processing first, fall back to CPU if needed
    try {
        return process_detections_gpu(predictions, masks);
    } catch (const std::exception& e) {
        logger::warn("GPU processing failed, falling back to CPU: {}", e.what());
        return process_detections_cpu(predictions, masks);
    }
}

// GPU-based detection processing
std::vector<Detection> PlugAndPlayYoloPostprocessorOp::process_detections_gpu(
    const std::shared_ptr<holoscan::Tensor>& predictions,
    const std::shared_ptr<holoscan::Tensor>& masks) {
    
    // TODO: Implement GPU-based processing
    // For now, fall back to CPU processing
    logger::debug("GPU processing not yet implemented, using CPU fallback");
    return process_detections_cpu(predictions, masks);
}

// CPU-based detection processing
std::vector<Detection> PlugAndPlayYoloPostprocessorOp::process_detections_cpu(
    const std::shared_ptr<holoscan::Tensor>& predictions,
    const std::shared_ptr<holoscan::Tensor>& masks) {
    
    std::vector<Detection> detections;
    
    // Get tensor dimensions
    auto pred_shape = predictions->shape();
    if (pred_shape.size() != 3) {
        throw std::runtime_error("Invalid predictions tensor shape: expected 3D");
    }
    
    int batch_size = pred_shape[0];
    int num_detections = pred_shape[1];
    int feature_size = pred_shape[2];
    
    logger::debug("Processing {} detections with {} features", num_detections, feature_size);
    
    // Get prediction data
    auto* pred_data = static_cast<const float*>(predictions->data());
    if (!pred_data) {
        throw std::runtime_error("Invalid predictions tensor data");
    }
    
    // Process each detection
    for (int i = 0; i < num_detections; ++i) {
        int offset = i * feature_size;
        
        // Extract box coordinates (normalized)
        float x_center = pred_data[offset + 0];
        float y_center = pred_data[offset + 1];
        float width = pred_data[offset + 2];
        float height = pred_data[offset + 3];
        
        // Convert to xyxy format
        float x1 = x_center - width / 2.0f;
        float y1 = y_center - height / 2.0f;
        float x2 = x_center + width / 2.0f;
        float y2 = y_center + height / 2.0f;
        
        // Get object confidence
        float obj_conf = pred_data[offset + 4];
        
        // Find best class
        float max_class_conf = 0.0f;
        int best_class = 0;
        
        for (int j = 0; j < static_cast<int>(class_metadata_.size()); ++j) {
            float class_conf = pred_data[offset + 5 + j];
            if (class_conf > max_class_conf) {
                max_class_conf = class_conf;
                best_class = j;
            }
        }
        
        // Calculate total confidence
        float total_conf = obj_conf * max_class_conf;
        
        // Apply confidence threshold
        if (total_conf < confidence_threshold_) continue;
        
        // Create detection
        Detection detection;
        detection.x1 = x1;
        detection.y1 = y1;
        detection.x2 = x2;
        detection.y2 = y2;
        detection.score = total_conf;
        detection.class_id = best_class;
        
        // Update visualization properties
        update_detection_visualization(detection);
        
        // Validate detection
        if (!detection.is_valid()) {
            logger::warn("Skipping invalid detection: {}", detection.validate());
            continue;
        }
        
        detections.push_back(detection);
    }
    
    // Apply NMS
    detections = plug_and_play_utils::apply_nms(detections, nms_threshold_);
    
    // Sort by confidence
    plug_and_play_utils::sort_by_confidence(detections);
    
    // Limit to max detections
    if (static_cast<int>(detections.size()) > max_detections_) {
        detections.resize(max_detections_);
    }
    
    logger::debug("Processed {} valid detections", detections.size());
    return detections;
}

// Output creation
DetectionTensorMap PlugAndPlayYoloPostprocessorOp::create_plug_and_play_output(
    const std::vector<Detection>& detections) {
    
    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    // Create detection tensor map
    DetectionTensorMap output = plug_and_play_utils::create_detection_tensor_map(
        detections, frame_counter_, timestamp);
    
    // Add processing metadata
    output.metadata["operator"] = "PlugAndPlayYoloPostprocessorOp";
    output.metadata["version"] = "1.0";
    output.metadata["format"] = "plug_and_play";
    output.metadata["confidence_threshold"] = std::to_string(confidence_threshold_);
    output.metadata["nms_threshold"] = std::to_string(nms_threshold_);
    output.metadata["max_detections"] = std::to_string(max_detections_);
    output.metadata["enable_masks"] = enable_masks_ ? "true" : "false";
    output.metadata["enable_labels"] = enable_labels_ ? "true" : "false";
    
    return output;
}

// Utility methods
void PlugAndPlayYoloPostprocessorOp::update_detection_visualization(Detection& detection) {
    calculate_text_position(detection);
    apply_color_scheme(detection);
}

void PlugAndPlayYoloPostprocessorOp::calculate_text_position(Detection& detection) {
    // Position text at top-left corner of bounding box
    detection.text_x = detection.x1;
    detection.text_y = detection.y1;
    detection.text_size = 0.04f; // Default text size
}

void PlugAndPlayYoloPostprocessorOp::apply_color_scheme(Detection& detection) {
    // Get color from class metadata
    if (detection.class_id >= 0 && detection.class_id < static_cast<int>(class_metadata_.size())) {
        const auto& class_info = class_metadata_[detection.class_id];
        detection.color_r = class_info.color[0];
        detection.color_g = class_info.color[1];
        detection.color_b = class_info.color[2];
        detection.color_a = class_info.color[3];
    } else {
        // Default color (white)
        detection.color_r = 1.0f;
        detection.color_g = 1.0f;
        detection.color_b = 1.0f;
        detection.color_a = 1.0f;
    }
}

// Validation
bool PlugAndPlayYoloPostprocessorOp::validate_inputs(
    const std::shared_ptr<holoscan::Tensor>& predictions,
    const std::shared_ptr<holoscan::Tensor>& masks) {
    
    if (!predictions) {
        logger::error("Predictions tensor is null");
        return false;
    }
    
    auto pred_shape = predictions->shape();
    if (pred_shape.size() != 3) {
        logger::error("Invalid predictions tensor shape: expected 3D, got {}D", pred_shape.size());
        return false;
    }
    
    if (pred_shape[0] != 1) {
        logger::error("Invalid batch size: expected 1, got {}", pred_shape[0]);
        return false;
    }
    
    if (pred_shape[2] < 5 + static_cast<int64_t>(class_metadata_.size())) {
        logger::error("Invalid feature size: expected at least {}, got {}", 
                     5 + class_metadata_.size(), pred_shape[2]);
        return false;
    }
    
    // Masks tensor is optional, but if provided, validate it
    if (masks) {
        auto mask_shape = masks->shape();
        if (mask_shape.size() != 4) {
            logger::error("Invalid masks tensor shape: expected 4D, got {}D", mask_shape.size());
            return false;
        }
    }
    
    return true;
}

// Error handling
void PlugAndPlayYoloPostprocessorOp::handle_processing_error(const std::string& error_message) {
    logger::error("PlugAndPlayYoloPostprocessorOp error: {}", error_message);
    // Could add error reporting, metrics, etc. here
}

void PlugAndPlayYoloPostprocessorOp::emit_empty_output(holoscan::OutputContext& op_output) {
    // Create empty detection tensor map
    std::vector<Detection> empty_detections;
    DetectionTensorMap empty_output = plug_and_play_utils::create_detection_tensor_map(
        empty_detections, frame_counter_, -1);
    
    // Add error metadata
    empty_output.metadata["error"] = "true";
    empty_output.metadata["error_time"] = std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    
    op_output.emit(empty_output, "out");
    logger::debug("Emitted empty output due to processing error");
}

// Configuration methods
void PlugAndPlayYoloPostprocessorOp::set_class_metadata(const std::vector<ClassMetadata>& classes) {
    class_metadata_ = classes;
    logger::info("Set {} class metadata entries", classes.size());
}

void PlugAndPlayYoloPostprocessorOp::set_confidence_threshold(float threshold) {
    confidence_threshold_ = threshold;
    logger::info("Set confidence threshold to {}", threshold);
}

void PlugAndPlayYoloPostprocessorOp::set_nms_threshold(float threshold) {
    nms_threshold_ = threshold;
    logger::info("Set NMS threshold to {}", threshold);
}

void PlugAndPlayYoloPostprocessorOp::set_max_detections(int max_detections) {
    max_detections_ = max_detections;
    logger::info("Set max detections to {}", max_detections);
}

void PlugAndPlayYoloPostprocessorOp::set_enable_masks(bool enable) {
    enable_masks_ = enable;
    logger::info("Set enable masks to {}", enable);
}

void PlugAndPlayYoloPostprocessorOp::set_enable_labels(bool enable) {
    enable_labels_ = enable;
    logger::info("Set enable labels to {}", enable);
}

// Default class metadata initialization
void PlugAndPlayYoloPostprocessorOp::initialize_default_class_metadata() {
    // Default class metadata for urology use case
    std::vector<ClassMetadata> default_classes = {
        {0, "Background", {0.0f, 0.0f, 0.0f, 0.0f}, true},
        {1, "Spleen", {0.1451f, 0.9412f, 0.6157f, 0.8f}, true},
        {2, "Left_Kidney", {0.8941f, 0.1176f, 0.0941f, 0.8f}, true},
        {3, "Left_Renal_Artery", {1.0000f, 0.8039f, 0.1529f, 0.8f}, true},
        {4, "Left_Renal_Vein", {0.0039f, 0.9373f, 1.0000f, 0.8f}, true},
        {5, "Left_Ureter", {0.9569f, 0.9019f, 0.1569f, 0.8f}, true},
        {6, "Left_Lumbar_Vein", {0.0157f, 0.4549f, 0.4509f, 0.8f}, true},
        {7, "Left_Adrenal_Vein", {0.8941f, 0.5647f, 0.0706f, 0.8f}, true},
        {8, "Left_Gonadal_Vein", {0.5019f, 0.1059f, 0.4471f, 0.8f}, true},
        {9, "Psoas_Muscle", {1.0000f, 1.0000f, 1.0000f, 0.8f}, true},
        {10, "Colon", {0.4314f, 0.4863f, 1.0000f, 0.8f}, true},
        {11, "Abdominal_Aorta", {0.6784f, 0.4941f, 0.2745f, 0.8f}, true}
    };
    
    set_class_metadata(default_classes);
    logger::info("Initialized default class metadata for urology use case");
}

} // namespace urology 