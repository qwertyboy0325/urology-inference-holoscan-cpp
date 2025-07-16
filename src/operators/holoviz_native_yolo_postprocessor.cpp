#include "operators/holoviz_native_yolo_postprocessor.hpp"
#include "operators/holoviz_common_types.hpp"
#include "utils/yolo_utils.hpp"

#include <holoscan/core/operator.hpp>
#include <holoscan/core/parameter.hpp>
#include <holoscan/core/operator_spec.hpp>
#include <holoscan/core/io_context.hpp>
#include <holoscan/core/execution_context.hpp>
#include <holoscan/operators/holoviz/holoviz.hpp>

#include <iostream>
#include <algorithm>
#include <numeric>
#include <sstream> // Added for std::ostringstream
#include <iomanip> // Added for std::fixed and std::setprecision

namespace urology {

void HolovizNativeYoloPostprocessor::setup(holoscan::OperatorSpec& spec) {
    // Input/Output specifications
    spec.input<holoscan::Tensor>("in");
    spec.output<holoscan::TensorMap>("out");
    spec.output<holoscan::Tensor>("output_tensors");
    spec.output<std::vector<holoscan::ops::HolovizOp::InputSpec>>("output_input_specs");
    
    // Parameters - using correct Holoscan parameter syntax
    spec.param(confidence_threshold_, "confidence_threshold", "Confidence threshold for detection filtering");
    spec.param(nms_threshold_, "nms_threshold", "NMS threshold for duplicate removal");
    spec.param(label_dict_, "label_dict", "Label dictionary mapping class IDs to label information");
    spec.param(color_lut_, "color_lut", "Color lookup table for visualization");
    spec.param(input_width_, "input_width", "Input image width");
    spec.param(input_height_, "input_height", "Input image height");
    spec.param(enable_gpu_processing_, "enable_gpu_processing", "Enable GPU processing for tensor operations");
    spec.param(enable_debug_output_, "enable_debug_output", "Enable debug output and logging");
}

void HolovizNativeYoloPostprocessor::initialize() {
    // Update parameter caches
    update_caches();
    
    // Initialize static configuration
    initialize_static_config();
    
    if (enable_debug_output_cache_) {
        UROLOGY_LOG_INFO("HolovizNativeYoloPostprocessor initialized successfully");
        std::ostringstream oss;
        oss << "Input dimensions: " << input_width_cache_ << "x" << input_height_cache_;
        UROLOGY_LOG_INFO(oss.str());
        oss.str("");
        oss << "Label dictionary size: " << label_dict_cache_.size();
        UROLOGY_LOG_INFO(oss.str());
        oss.str("");
        oss << "Color LUT size: " << color_lut_cache_.size();
        UROLOGY_LOG_INFO(oss.str());
    }
}

void HolovizNativeYoloPostprocessor::compute(holoscan::InputContext& op_input, 
                                            holoscan::OutputContext& op_output, 
                                            holoscan::ExecutionContext& context) {
    PERF_START("compute");
    
    try {
        // Update parameter caches if needed
        update_caches();
        
        // Get input tensor
        auto input_tensor = op_input.receive<holoscan::Tensor>("input_tensor");
        if (!input_tensor) {
            UROLOGY_LOG_ERROR("No input tensor received");
            PERF_STOP("compute");
            return;
        }
        
        if (enable_debug_output_cache_) {
            // Print shape as comma-separated string
            const auto& shape = input_tensor->shape();
            std::ostringstream oss;
            oss << "Processing input tensor with shape: [";
            for (size_t i = 0; i < shape.size(); ++i) {
                oss << shape[i];
                if (i + 1 < shape.size()) oss << ", ";
            }
            oss << "]";
            UROLOGY_LOG_DEBUG(oss.str());
        }
        
        // Process input tensor to extract detections
        auto detections = process_input_tensor(*input_tensor);
        
        if (enable_debug_output_cache_) {
            log_detection_summary(detections);
        }
        
        // Generate HolovizOp-compatible tensors
        generate_holoviz_tensors(detections, op_output);
        
        PERF_STOP("compute");
        
        if (enable_debug_output_cache_) {
            UROLOGY_LOG_DEBUG("Compute completed successfully");
        }
        
    } catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "Error in compute: " << e.what();
        UROLOGY_LOG_ERROR(oss.str());
        PERF_STOP("compute");
    }
}

std::vector<Detection> HolovizNativeYoloPostprocessor::process_input_tensor(const holoscan::Tensor& input_tensor) {
    PERF_START("process_input_tensor");
    
    // For now, we'll create some dummy detections for testing
    // In a real implementation, this would parse the YOLO output tensors
    std::vector<Detection> detections;
    
    // Create dummy detections for testing
    detections.emplace_back(std::vector<float>{0.1f, 0.2f, 0.3f, 0.4f}, 0.85f, 1);
    detections.emplace_back(std::vector<float>{0.5f, 0.6f, 0.7f, 0.8f}, 0.92f, 2);
    
    // Filter detections by confidence threshold
    auto filtered_detections = filter_detections(detections);
    
    // Apply NMS
    auto nms_detections = apply_nms(filtered_detections, nms_threshold_cache_[0]);
    
    // Normalize coordinates
    normalize_coordinates(nms_detections);
    
    PERF_STOP("process_input_tensor");
    
    return nms_detections;
}

std::vector<Detection> HolovizNativeYoloPostprocessor::extract_detections(const holoscan::Tensor& outputs_tensor, 
                                                                         const holoscan::Tensor& proto_tensor) {
    // This is a placeholder implementation
    // In a real implementation, this would parse the YOLO output tensors
    // to extract bounding boxes, confidence scores, and class IDs
    return std::vector<Detection>{};
}

std::vector<Detection> HolovizNativeYoloPostprocessor::filter_detections(const std::vector<Detection>& detections) {
    std::vector<Detection> filtered;
    
    for (const auto& detection : detections) {
        if (detection.confidence >= confidence_threshold_cache_[0]) {
            filtered.push_back(detection);
        }
    }
    
    return filtered;
}

std::vector<Detection> HolovizNativeYoloPostprocessor::apply_nms(const std::vector<Detection>& detections, 
                                                                 float nms_threshold) {
    if (detections.empty()) {
        return detections;
    }
    
    std::vector<Detection> result = detections;
    
    // Sort by confidence (descending)
    std::sort(result.begin(), result.end(), 
              [](const Detection& a, const Detection& b) {
                  return a.confidence > b.confidence;
              });
    
    std::vector<bool> keep(result.size(), true);
    
    for (size_t i = 0; i < result.size(); ++i) {
        if (!keep[i]) continue;
        
        for (size_t j = i + 1; j < result.size(); ++j) {
            if (!keep[j]) continue;
            
            // Only apply NMS to detections of the same class
            if (result[i].class_id == result[j].class_id) {
                float iou = urology::yolo_utils::calculate_iou(result[i].box, result[j].box);
                if (iou > nms_threshold) {
                    keep[j] = false;
                }
            }
        }
    }
    
    std::vector<Detection> nms_result;
    for (size_t i = 0; i < result.size(); ++i) {
        if (keep[i]) {
            nms_result.push_back(result[i]);
        }
    }
    
    return nms_result;
}

void HolovizNativeYoloPostprocessor::normalize_coordinates(std::vector<Detection>& detections) {
    for (auto& detection : detections) {
        // Ensure coordinates are in [0, 1] range
        for (auto& coord : detection.box) {
            coord = std::max(0.0f, std::min(1.0f, coord));
        }
    }
}

void HolovizNativeYoloPostprocessor::generate_holoviz_tensors(const std::vector<Detection>& detections,
                                                             holoscan::OutputContext& op_output) {
    PERF_START("generate_holoviz_tensors");
    
    try {
        // Create compatible tensors object
        HolovizCompatibleTensors compatible_tensors;
        compatible_tensors.label_dict = label_dict_cache_;
        compatible_tensors.color_lut = color_lut_cache_;
        
        // Generate different types of tensors
        generate_bounding_box_tensors(detections, compatible_tensors);
        generate_text_tensors(detections, compatible_tensors);
        generate_mask_tensors(detections, compatible_tensors);
        
        // Get all tensors and emit them
        auto all_tensors = compatible_tensors.get_all_tensors();
        for (const auto& tensor : all_tensors) {
            op_output.emit(tensor, "output_tensors");
        }
        
        // Generate and emit dynamic InputSpecs
        auto dynamic_specs = generate_dynamic_input_specs(detections);
        op_output.emit(dynamic_specs, "output_input_specs");
        
        PERF_STOP("generate_holoviz_tensors");
        
    } catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "Error generating HolovizOp tensors: " << e.what();
        UROLOGY_LOG_ERROR(oss.str());
        PERF_STOP("generate_holoviz_tensors");
    }
}

void HolovizNativeYoloPostprocessor::generate_bounding_box_tensors(const std::vector<Detection>& detections,
                                                                  HolovizCompatibleTensors& compatible_tensors) {
    if (detections.empty()) return;
    
    std::vector<std::vector<float>> boxes;
    for (const auto& detection : detections) {
        boxes.push_back(detection.box);
    }
    
    compatible_tensors.add_rectangle_tensor("bounding_boxes", boxes);
}

void HolovizNativeYoloPostprocessor::generate_text_tensors(const std::vector<Detection>& detections,
                                                          HolovizCompatibleTensors& compatible_tensors) {
    if (detections.empty()) return;
    
    std::vector<std::vector<float>> text_positions;
    for (const auto& detection : detections) {
        // Position text at the top-left corner of the bounding box
        text_positions.push_back({detection.box[0], detection.box[1]});
    }
    
    compatible_tensors.add_text_tensor("labels", text_positions);
}

void HolovizNativeYoloPostprocessor::generate_mask_tensors(const std::vector<Detection>& detections,
                                                          HolovizCompatibleTensors& compatible_tensors) {
    // This is a placeholder implementation
    // In a real implementation, this would generate segmentation masks
    // based on the proto tensor from YOLO
    for (const auto& detection : detections) {
        // Placeholder: create a simple mask tensor
        // In reality, this would use the proto tensor to generate proper masks
    }
}

std::vector<holoscan::ops::HolovizOp::InputSpec> HolovizNativeYoloPostprocessor::generate_dynamic_input_specs(
    const std::vector<Detection>& detections) {
    return urology::create_dynamic_input_specs(detections, label_dict_cache_);
}

void HolovizNativeYoloPostprocessor::update_caches() {
    // Update cached parameters - check if they are set before getting values
    try {
        confidence_threshold_cache_ = confidence_threshold_.get();
    } catch (...) {
        confidence_threshold_cache_ = {kDefaultConfidenceThreshold};
    }
    
    try {
        nms_threshold_cache_ = nms_threshold_.get();
    } catch (...) {
        nms_threshold_cache_ = {kDefaultNmsThreshold};
    }
    
    try {
        label_dict_cache_ = label_dict_.get();
    } catch (...) {
        label_dict_cache_ = LabelDict{};
    }
    
    try {
        color_lut_cache_ = color_lut_.get();
    } catch (...) {
        color_lut_cache_ = ColorLUT{};
    }
    
    try {
        input_width_cache_ = input_width_.get();
    } catch (...) {
        input_width_cache_ = kDefaultInputWidth;
    }
    
    try {
        input_height_cache_ = input_height_.get();
    } catch (...) {
        input_height_cache_ = kDefaultInputHeight;
    }
    
    try {
        enable_gpu_processing_cache_ = enable_gpu_processing_.get();
    } catch (...) {
        enable_gpu_processing_cache_ = true;
    }
    
    try {
        enable_debug_output_cache_ = enable_debug_output_.get();
    } catch (...) {
        enable_debug_output_cache_ = false;
    }
}

void HolovizNativeYoloPostprocessor::initialize_static_config() {
    // Initialize static tensor configuration
    static_tensor_config_ = urology::create_static_tensor_config(label_dict_cache_);
    
    if (enable_debug_output_cache_) {
        std::ostringstream oss;
        oss << "Initialized static tensor configuration with " << static_tensor_config_.size() << " tensor specs";
        UROLOGY_LOG_INFO(oss.str());
    }
}

void HolovizNativeYoloPostprocessor::log_detection_summary(const std::vector<Detection>& detections) {
    std::ostringstream oss;
    oss << "Processed " << detections.size() << " detections";
    UROLOGY_LOG_DEBUG(oss.str());
    
    for (size_t i = 0; i < detections.size() && i < 5; ++i) {
        const auto& det = detections[i];
        oss.str("");
        oss << "Detection " << i << ": class=" << det.class_id 
            << ", confidence=" << std::fixed << std::setprecision(3) << det.confidence
            << ", box=[" << std::fixed << std::setprecision(3) 
            << det.box[0] << ", " << det.box[1] << ", " << det.box[2] << ", " << det.box[3] << "]";
        UROLOGY_LOG_DEBUG(oss.str());
    }
}

void HolovizNativeYoloPostprocessor::validate_parameters() {
    if (confidence_threshold_cache_.empty()) {
        throw std::runtime_error("Confidence threshold is empty");
    }
    if (nms_threshold_cache_.empty()) {
        throw std::runtime_error("NMS threshold is empty");
    }
    if (input_width_cache_ <= 0 || input_height_cache_ <= 0) {
        throw std::runtime_error("Invalid input dimensions");
    }
}

} // namespace urology 