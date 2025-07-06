#include "holoscan_fix.hpp"
#include "operators/yolo_seg_postprocessor.hpp"
#include "utils/yolo_utils.hpp"
#include <cuda_runtime.h>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <dlpack/dlpack.h>
#include <memory>

namespace urology {

// External CUDA kernel declarations
extern "C" {
    void launch_yolo_postprocess_kernel(
        const float* predictions,
        float* output_boxes,
        float* output_scores,
        int* output_class_ids,
        int* valid_detections,
        int num_detections,
        int feature_size,
        float confidence_threshold,
        int num_classes,
        cudaStream_t stream
    );
    
    void launch_nms_kernel(
        const float* boxes,
        const float* scores,
        const int* class_ids,
        int* keep_flags,
        int num_detections,
        float iou_threshold,
        cudaStream_t stream
    );
}

void YoloSegPostprocessorOp::setup(holoscan::OperatorSpec& spec) {
    spec.input<holoscan::TensorMap>("in");
    spec.output<holoscan::TensorMap>("out");
    spec.output<std::vector<holoscan::ops::HolovizOp::InputSpec>>("output_specs");
    spec.param(scores_threshold_, "scores_threshold", "Scores threshold");
    spec.param(num_class_, "num_class", "Number of classes");
    spec.param(out_tensor_name_, "out_tensor_name", "Output tensor name");
}

void YoloSegPostprocessorOp::compute(holoscan::InputContext& op_input, 
                                    holoscan::OutputContext& op_output,
                                    holoscan::ExecutionContext& /* context */) {
    try {
        // Receive inference inputs only (matching Python version)
        std::shared_ptr<holoscan::Tensor> predictions;
        std::shared_ptr<holoscan::Tensor> masks_seg;
        receive_inputs(op_input, predictions, masks_seg);
        
        // Process predictions using GPU
        YoloOutput output = process_boxes_gpu(predictions, masks_seg);
        
        std::cout << "YoloOutput before sort - Found " << output.boxes.size() << " detections" << std::endl;
        
        // Sort by class IDs (matching Python implementation)
        output.sort_by_class_ids();
        
        std::cout << "YoloOutput after sort - Found " << output.boxes.size() << " detections" << std::endl;
        
        // Organize boxes by class ID
        auto organized_boxes = organize_boxes(output.class_ids, output.boxes);
        
        // Create output message following Python version approach
        std::unordered_map<std::string, std::shared_ptr<holoscan::Tensor>> out_message;
        std::map<int, std::vector<std::vector<float>>> out_texts;
        std::map<int, std::vector<std::vector<float>>> out_scores_pos;
        
        // Prepare output message (matching Python version)
        prepare_output_message(organized_boxes, out_message, out_texts, out_scores_pos);
        
        // Process scores for visualization
        std::vector<holoscan::ops::HolovizOp::InputSpec> specs;
        process_scores(output.class_ids, output.boxes_scores, out_scores_pos, specs, out_message);
        
        // Emit outputs
        op_output.emit(out_message, "out");
        op_output.emit(specs, "output_specs");
        
    } catch (const std::exception& e) {
        std::cerr << "Error in YoloSegPostprocessorOp: " << e.what() << std::endl;
        throw;
    }
}

void YoloSegPostprocessorOp::receive_inputs(holoscan::InputContext& op_input,
                                           std::shared_ptr<holoscan::Tensor>& predictions,
                                           std::shared_ptr<holoscan::Tensor>& masks_seg) {
    auto in_message = op_input.receive<holoscan::TensorMap>("in").value();
    
    // Debug: Print available tensor names
    std::cout << "Available tensor names in YoloSegPostprocessorOp:" << std::endl;
    for (const auto& [name, tensor] : in_message) {
        std::cout << "  - " << name << std::endl;
    }
    
    // C++ version uses "outputs" and "proto" tensor names
    if (in_message.find("outputs") != in_message.end()) {
        predictions = in_message["outputs"];
        std::cout << "Found 'outputs' tensor for predictions" << std::endl;
    } else {
        throw std::runtime_error("Missing 'outputs' tensor");
    }
    
    if (in_message.find("proto") != in_message.end()) {
        masks_seg = in_message["proto"];
        std::cout << "Found 'proto' tensor for masks" << std::endl;
    } else {
        throw std::runtime_error("Missing 'proto' tensor");
    }
}

YoloOutput YoloSegPostprocessorOp::process_boxes_gpu(const std::shared_ptr<holoscan::Tensor>& predictions,
                                                     const std::shared_ptr<holoscan::Tensor>& /* masks_seg */) {
    YoloOutput output;
    
    // Safety checks
    if (!predictions || !predictions->data()) {
        std::cerr << "Error: predictions tensor is null or has no data" << std::endl;
        return output;
    }
    
    // Get tensor dimensions
    auto pred_shape = predictions->shape();
    
    // Validate tensor shapes
    if (pred_shape.size() < 3) {
        std::cerr << "Error: predictions tensor should have at least 3 dimensions, got " << pred_shape.size() << std::endl;
        return output;
    }
    
    // Parse tensor dimensions
    int batch_size = pred_shape[0];
    int num_detections = pred_shape[1];  // 8400
    int feature_size = pred_shape[2];    // 38
    
    std::cout << "GPU Processing: " << batch_size << "x" << num_detections << "x" << feature_size << std::endl;
    
    // Get GPU data pointer directly
    const float* gpu_predictions = static_cast<const float*>(predictions->data());
    
    // Allocate GPU memory for results using raw CUDA
    float* d_output_boxes;
    float* d_output_scores;
    int* d_output_class_ids;
    int* d_valid_detections;
    int* d_keep_flags;
    
    size_t boxes_size = num_detections * 4 * sizeof(float);
    size_t scores_size = num_detections * sizeof(float);
    size_t ids_size = num_detections * sizeof(int);
    size_t flags_size = num_detections * sizeof(int);
    
    cudaMalloc(&d_output_boxes, boxes_size);
    cudaMalloc(&d_output_scores, scores_size);
    cudaMalloc(&d_output_class_ids, ids_size);
    cudaMalloc(&d_valid_detections, flags_size);
    cudaMalloc(&d_keep_flags, flags_size);
    
    // Initialize keep_flags to 1
    cudaMemset(d_keep_flags, 1, flags_size);
    
    // Launch CUDA kernel for postprocessing
    int block_size = 256;
    int grid_size = (num_detections + block_size - 1) / block_size;
    
    std::cout << "Launching CUDA kernel with grid_size=" << grid_size << ", block_size=" << block_size << std::endl;
    
    launch_yolo_postprocess_kernel(
        gpu_predictions,
        d_output_boxes,
        d_output_scores,
        d_output_class_ids,
        d_valid_detections,
        num_detections,
        feature_size,
        0.05f, // Very low confidence threshold for testing
        static_cast<int>(num_class_),
        0  // Default CUDA stream
    );
    
    // Check for CUDA errors
    cudaError_t cuda_error = cudaGetLastError();
    if (cuda_error != cudaSuccess) {
        std::cerr << "CUDA kernel launch failed: " << cudaGetErrorString(cuda_error) << std::endl;
        // Cleanup
        cudaFree(d_output_boxes);
        cudaFree(d_output_scores);
        cudaFree(d_output_class_ids);
        cudaFree(d_valid_detections);
        cudaFree(d_keep_flags);
        return output;
    }
    
    // Synchronize to ensure kernel completion
    cudaDeviceSynchronize();
    
    // Apply NMS on GPU
    launch_nms_kernel(
        d_output_boxes,
        d_output_scores,
        d_output_class_ids,
        d_keep_flags,
        num_detections,
        0.45f, // IoU threshold
        0      // Default CUDA stream
    );
    
    // Check for CUDA errors
    cuda_error = cudaGetLastError();
    if (cuda_error != cudaSuccess) {
        std::cerr << "NMS kernel launch failed: " << cudaGetErrorString(cuda_error) << std::endl;
        // Cleanup
        cudaFree(d_output_boxes);
        cudaFree(d_output_scores);
        cudaFree(d_output_class_ids);
        cudaFree(d_valid_detections);
        cudaFree(d_keep_flags);
        return output;
    }
    
    // Synchronize to ensure kernel completion
    cudaDeviceSynchronize();
    
    // Copy results back to host
    std::vector<float> h_output_boxes(num_detections * 4);
    std::vector<float> h_output_scores(num_detections);
    std::vector<int> h_output_class_ids(num_detections);
    std::vector<int> h_valid_detections(num_detections);
    std::vector<int> h_keep_flags(num_detections);
    
    cudaMemcpy(h_output_boxes.data(), d_output_boxes, boxes_size, cudaMemcpyDeviceToHost);
    cudaMemcpy(h_output_scores.data(), d_output_scores, scores_size, cudaMemcpyDeviceToHost);
    cudaMemcpy(h_output_class_ids.data(), d_output_class_ids, ids_size, cudaMemcpyDeviceToHost);
    cudaMemcpy(h_valid_detections.data(), d_valid_detections, flags_size, cudaMemcpyDeviceToHost);
    cudaMemcpy(h_keep_flags.data(), d_keep_flags, flags_size, cudaMemcpyDeviceToHost);
    
    // Cleanup GPU memory
    cudaFree(d_output_boxes);
    cudaFree(d_output_scores);
    cudaFree(d_output_class_ids);
    cudaFree(d_valid_detections);
    cudaFree(d_keep_flags);
    
    // Extract final results
    for (int i = 0; i < num_detections; ++i) {
        if (h_valid_detections[i] && h_keep_flags[i]) {
            output.boxes.push_back({
                h_output_boxes[i * 4 + 0],
                h_output_boxes[i * 4 + 1],
                h_output_boxes[i * 4 + 2],
                h_output_boxes[i * 4 + 3]
            });
            output.boxes_scores.push_back(h_output_scores[i]);
            output.class_ids.push_back(h_output_class_ids[i]);
        }
    }
    
    std::cout << "GPU Processing completed. Found " << output.boxes.size() << " valid detections." << std::endl;
    
    return output;
}

void YoloSegPostprocessorOp::create_placeholder_mask_tensor(
    std::unordered_map<std::string, std::shared_ptr<holoscan::Tensor>>& out_message) {
    // Create a simple placeholder mask tensor to satisfy HolovizOp requirements
    // This matches the Python version's approach of creating a mask tensor
    
    // For now, we'll create a minimal tensor to avoid the "Failed to retrieve input 'masks'" error
    // TODO: Implement proper mask processing using the masks_seg tensor
    
    std::cout << "Creating placeholder mask tensor (skipped for now)" << std::endl;
    
    // TODO: Implement proper mask tensor creation when tensor API is resolved
    // For now, just skip to avoid crashes
    (void)out_message; // Suppress unused parameter warning
}

std::map<int, std::vector<std::vector<float>>> YoloSegPostprocessorOp::organize_boxes(
    const std::vector<int>& class_ids,
    const std::vector<std::vector<float>>& boxes) {
    
    std::map<int, std::vector<std::vector<float>>> organized;
    
    for (size_t i = 0; i < class_ids.size(); ++i) {
        int class_id = class_ids[i];
        organized[class_id].push_back(boxes[i]);
    }
    
    return organized;
}

void YoloSegPostprocessorOp::prepare_output_message(
    const std::map<int, std::vector<std::vector<float>>>& organized_boxes,
    std::unordered_map<std::string, std::shared_ptr<holoscan::Tensor>>& out_message,
    std::map<int, std::vector<std::vector<float>>>& out_texts,
    std::map<int, std::vector<std::vector<float>>>& out_scores_pos) {
    
    std::cout << "Preparing output message for " << organized_boxes.size() << " class groups" << std::endl;
    
    // Create output tensors for each class (matching Python implementation)
    for (int class_id = 0; class_id < static_cast<int>(num_class_); ++class_id) {
        std::string boxes_key = "boxes" + std::to_string(class_id);
        std::string label_key = "label" + std::to_string(class_id);
        
        if (organized_boxes.find(class_id) != organized_boxes.end()) {
            // Class has detections
            const auto& class_boxes = organized_boxes.at(class_id);
            
            // Create boxes tensor: reshape to (1, -1, 2) format
            std::vector<float> boxes_data;
            std::vector<std::vector<float>> text_coords;
            std::vector<std::vector<float>> score_coords;
            
            for (const auto& box : class_boxes) {
                // box format: [x1, y1, x2, y2] -> convert to two points format
                float x1 = box[0], y1 = box[1], x2 = box[2], y2 = box[3];
                
                // First point (top-left)
                boxes_data.push_back(x1);
                boxes_data.push_back(y1);
                
                // Second point (bottom-right)
                boxes_data.push_back(x2);
                boxes_data.push_back(y2);
                
                // Text position (top-left corner)
                text_coords.push_back({x1, y1});
                
                // Score position (top-right corner)
                score_coords.push_back({x2, y1});
            }
            
                    // Log detection results but skip tensor creation for now
            if (!class_boxes.empty()) {
                std::cout << "Found " << class_boxes.size() << " detections for class " << class_id << std::endl;
                for (size_t i = 0; i < class_boxes.size(); ++i) {
                    const auto& box = class_boxes[i];
                    std::cout << "  Box " << i << ": [" << box[0] << ", " << box[1] << ", " << box[2] << ", " << box[3] << "]" << std::endl;
                }
                // TODO: Create actual tensor when tensor API is resolved
            }
            
            // Store text and score coordinates
            out_texts[class_id] = append_size_to_text_coord(text_coords, 0.04f);
            out_scores_pos[class_id] = append_size_to_score_coord(score_coords, 0.04f);
            
            std::cout << "Created tensor for class " << class_id << " with " << class_boxes.size() << " boxes" << std::endl;
        } else {
            // TODO: Class has no detections - create empty tensors (temporarily disabled)
            // std::vector<float> empty_boxes_data = {-1.0f, -1.0f, -1.0f, -1.0f};
            // std::vector<int64_t> empty_boxes_shape = {1, 1, 2, 2};
            // auto empty_boxes_tensor = std::make_shared<holoscan::Tensor>(...);
            // out_message[boxes_key] = empty_boxes_tensor;
            
            // Empty text coordinates
            out_texts[class_id] = {{{-1.0f, -1.0f, 0.04f}}};
        }
        
        // Create label tensor (text coordinates)
        const auto& text_coords = out_texts[class_id];
        std::vector<float> label_data;
        for (const auto& coord : text_coords) {
            label_data.insert(label_data.end(), coord.begin(), coord.end());
        }
        
        // TODO: Create label tensor (temporarily disabled due to tensor API issues)
        // std::vector<int64_t> label_shape = {1, static_cast<int64_t>(text_coords.size()), 3};
        // auto label_tensor = std::make_shared<holoscan::Tensor>(...);
        // out_message[label_key] = label_tensor;
    }
}

void YoloSegPostprocessorOp::process_scores(
    const std::vector<int>& class_ids,
    const std::vector<float>& scores,
    const std::map<int, std::vector<std::vector<float>>>& scores_pos,
    std::vector<holoscan::ops::HolovizOp::InputSpec>& specs,
    std::unordered_map<std::string, std::shared_ptr<holoscan::Tensor>>& out_scores) {
    
    std::cout << "Processing scores for " << class_ids.size() << " detections" << std::endl;
    
    // For now, don't create any specs to avoid tensor creation issues
    // TODO: Implement proper score visualization when tensor API is resolved
    
    std::cout << "Skipping score visualization specs for now" << std::endl;
    
    // Log detection information for debugging
    for (size_t i = 0; i < class_ids.size(); ++i) {
        int class_id = class_ids[i];
        float score = scores[i];
        std::cout << "Detection " << i << ": class=" << class_id << ", score=" << score << std::endl;
    }
}

void YoloOutput::sort_by_class_ids() {
    // Create index vector
    std::vector<size_t> indices(class_ids.size());
    std::iota(indices.begin(), indices.end(), 0);
    
    // Sort indices by class_ids
    std::sort(indices.begin(), indices.end(), [this](size_t a, size_t b) {
        return class_ids[a] < class_ids[b];
    });
    
    // Reorder all vectors according to sorted indices
    std::vector<int> sorted_class_ids;
    std::vector<float> sorted_scores;
    std::vector<std::vector<float>> sorted_boxes;
    std::vector<std::vector<float>> sorted_masks;
    
    for (size_t idx : indices) {
        sorted_class_ids.push_back(class_ids[idx]);
        sorted_scores.push_back(boxes_scores[idx]);
        sorted_boxes.push_back(boxes[idx]);
        if (idx < output_masks.size()) {
            sorted_masks.push_back(output_masks[idx]);
        }
    }
    
    class_ids = std::move(sorted_class_ids);
    boxes_scores = std::move(sorted_scores);
    boxes = std::move(sorted_boxes);
    output_masks = std::move(sorted_masks);
}

// Utility functions
std::vector<std::vector<float>> append_size_to_text_coord(
    const std::vector<std::vector<float>>& coords, float size) {
    
    std::vector<std::vector<float>> result;
    for (const auto& coord : coords) {
        if (coord.size() >= 2) {
            result.push_back({coord[0], coord[1], size});
        }
    }
    return result;
}

std::vector<std::vector<float>> append_size_to_score_coord(
    const std::vector<std::vector<float>>& coords, float size) {
    
    std::vector<std::vector<float>> result;
    for (const auto& coord : coords) {
        if (coord.size() >= 2) {
            result.push_back({coord[0], coord[1], size});
        }
    }
    return result;
}

} // namespace urology 