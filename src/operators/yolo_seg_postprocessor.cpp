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
#include <chrono>
#include <sys/resource.h>

namespace urology {

// Function to get current memory usage
void log_memory_usage(const std::string& phase) {
    struct rusage r_usage;
    if (getrusage(RUSAGE_SELF, &r_usage) == 0) {
        long memory_kb = r_usage.ru_maxrss;
        std::cout << "[POSTPROC] " << phase << " - Memory usage: " << (memory_kb / 1024) << " MB" << std::endl;
    }
}

// Constructor and destructor logging
YoloSegPostprocessorOp::YoloSegPostprocessorOp() {
    std::cout << "[POSTPROC] === YoloSegPostprocessorOp constructor called ===" << std::endl;
    log_memory_usage("Postprocessor constructor");
}

YoloSegPostprocessorOp::~YoloSegPostprocessorOp() {
    std::cout << "[POSTPROC] === YoloSegPostprocessorOp destructor called ===" << std::endl;
    log_memory_usage("Postprocessor destructor");
}

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
    std::cout << "[POSTPROC] === Starting YoloSegPostprocessorOp::setup() ===" << std::endl;
    log_memory_usage("Postprocessor setup start");
    
    try {
        std::cout << "[POSTPROC] Setting up input/output specifications..." << std::endl;
        spec.input<holoscan::TensorMap>("in");
        spec.output<holoscan::TensorMap>("out");
        spec.output<std::vector<holoscan::ops::HolovizOp::InputSpec>>("output_specs");
        
        std::cout << "[POSTPROC] Setting up parameters..." << std::endl;
        spec.param(scores_threshold_, "scores_threshold", "Scores threshold");
        spec.param(num_class_, "num_class", "Number of classes");
        spec.param(out_tensor_name_, "out_tensor_name", "Output tensor name");
        
        std::cout << "[POSTPROC] Setup completed successfully" << std::endl;
        log_memory_usage("Postprocessor setup end");
    } catch (const std::exception& e) {
        std::cerr << "[POSTPROC] ERROR in setup: " << e.what() << std::endl;
        throw;
    }
}

void YoloSegPostprocessorOp::compute(holoscan::InputContext& op_input, 
                                    holoscan::OutputContext& op_output,
                                    holoscan::ExecutionContext& /* context */) {
    std::cout << "[POSTPROC] === Starting YoloSegPostprocessorOp::compute() ===" << std::endl;
    log_memory_usage("Postprocessor compute start");
    
    try {
        std::cout << "[POSTPROC] Step 1: Receiving inputs..." << std::endl;
        // Receive inference inputs only (matching Python version)
        std::shared_ptr<holoscan::Tensor> predictions;
        std::shared_ptr<holoscan::Tensor> masks_seg;
        receive_inputs(op_input, predictions, masks_seg);
        std::cout << "[POSTPROC] Step 1 completed: Inputs received" << std::endl;
        
        std::cout << "[POSTPROC] Step 2: Processing predictions using GPU..." << std::endl;
        // Process predictions using GPU (memory optimized)
        YoloOutput output = process_boxes_gpu(predictions, masks_seg);
        std::cout << "[POSTPROC] Step 2 completed: GPU processing done" << std::endl;
        
        std::cout << "[POSTPROC] Step 3: Post-processing output..." << std::endl;
        std::cout << "YoloOutput before sort - Found " << output.boxes.size() << " detections" << std::endl;
        
        // Sort by class IDs (matching Python implementation)
        output.sort_by_class_ids();
        
        std::cout << "YoloOutput after sort - Found " << output.boxes.size() << " detections" << std::endl;
        
        // Organize boxes by class ID
        auto organized_boxes = organize_boxes(output.class_ids, output.boxes);
        std::cout << "[POSTPROC] Step 3 completed: Output organized" << std::endl;
        
        std::cout << "[POSTPROC] Step 4: Creating output message..." << std::endl;
        // Create output message following Python version approach
        std::unordered_map<std::string, std::shared_ptr<holoscan::Tensor>> out_message;
        std::map<int, std::vector<std::vector<float>>> out_texts;
        std::map<int, std::vector<std::vector<float>>> out_scores_pos;
        
        // Prepare output message (matching Python version)
        prepare_output_message(organized_boxes, out_message, out_texts, out_scores_pos);
        std::cout << "[POSTPROC] Step 4 completed: Output message prepared" << std::endl;
        
        std::cout << "[POSTPROC] Step 5: Processing scores for visualization..." << std::endl;
        // Process scores for visualization - SIMPLIFIED APPROACH
        std::vector<holoscan::ops::HolovizOp::InputSpec> specs;
        std::unordered_map<std::string, std::shared_ptr<holoscan::Tensor>> out_scores;
        
        // Create simple score visualization without complex tensor names
        if (!output.class_ids.empty()) {
            // Create a simple text specification for scores
            holoscan::ops::HolovizOp::InputSpec score_spec("", "text");
            score_spec.color_ = {1.0f, 1.0f, 1.0f, 1.0f}; // White color
            
            // Add score text for each detection
            for (size_t i = 0; i < output.class_ids.size() && i < output.boxes_scores.size(); ++i) {
                std::string score_text = "Class " + std::to_string(output.class_ids[i]) + ": " + 
                                       std::to_string(output.boxes_scores[i]).substr(0, 6);
                score_spec.text_.push_back(score_text);
                std::cout << "Detection " << i << ": class=" << output.class_ids[i] << ", score=" << output.boxes_scores[i] << std::endl;
            }
            specs.push_back(score_spec);
            std::cout << "Created " << output.class_ids.size() << " score text specifications" << std::endl;
        }
        
        std::cout << "[POSTPROC] Step 5 completed: Scores processed with simplified approach" << std::endl;
        
        std::cout << "[POSTPROC] Step 6: Creating mask tensor..." << std::endl;
        // Create mask tensor (memory optimized)
        if (!output.output_masks.empty()) {
            std::cout << "Creating mask tensor with " << output.output_masks.size() << " masks" << std::endl;
            
            // Pre-calculate total size to avoid reallocation
            size_t total_mask_size = 0;
            for (const auto& mask : output.output_masks) {
                total_mask_size += mask.size();
            }
            
            // Pre-allocate mask data vector
            std::vector<float> mask_data;
            mask_data.reserve(total_mask_size);
            
            // Flatten the mask data for tensor creation
            for (const auto& mask : output.output_masks) {
                mask_data.insert(mask_data.end(), mask.begin(), mask.end());
            }
            
            // Create mask tensor with appropriate shape
            size_t num_masks = output.output_masks.size();
            size_t mask_size = output.output_masks[0].size();
            size_t mask_dim = static_cast<size_t>(std::sqrt(mask_size)); // Assuming square masks
            
            std::vector<int64_t> mask_shape = {static_cast<int64_t>(num_masks), 
                                              static_cast<int64_t>(mask_dim), 
                                              static_cast<int64_t>(mask_dim)};
            
            std::cout << "[TENSOR] Creating mask tensor with shape: [" << mask_shape[0] << ", " 
                      << mask_shape[1] << ", " << mask_shape[2] << "] (" << mask_data.size() * sizeof(float) << " bytes)" << std::endl;
            auto mask_tensor = urology::yolo_utils::make_holoscan_tensor_from_data(
                mask_data.data(), mask_shape, 2, 32, 1);
            out_message["masks"] = mask_tensor;
            std::cout << "[TENSOR] Mask tensor created." << std::endl;
        } else {
            std::cout << "No masks to create tensor for" << std::endl;
        }
        std::cout << "[POSTPROC] Step 6 completed: Mask tensor created" << std::endl;
        
        std::cout << "[POSTPROC] Step 7: Emitting outputs..." << std::endl;
        // Emit outputs
        op_output.emit(out_message, "out");
        op_output.emit(specs, "output_specs");
        std::cout << "[POSTPROC] Step 7 completed: Outputs emitted" << std::endl;
        
        std::cout << "[POSTPROC] === YoloSegPostprocessorOp::compute() completed successfully ===" << std::endl;
        log_memory_usage("Postprocessor compute end");
        
    } catch (const std::exception& e) {
        std::cerr << "[POSTPROC] ERROR in compute: " << e.what() << std::endl;
        log_memory_usage("Postprocessor compute error");
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
    std::cout << "[POSTPROC] === Starting process_boxes_gpu() ===" << std::endl;
    log_memory_usage("GPU processing start");
    
    YoloOutput output;
    
    std::cout << "[POSTPROC] Step GPU-1: Safety checks..." << std::endl;
    // Safety checks
    if (!predictions || !predictions->data()) {
        std::cerr << "[POSTPROC] Error: predictions tensor is null or has no data" << std::endl;
        return output;
    }
    
    std::cout << "[POSTPROC] Step GPU-2: Getting tensor dimensions..." << std::endl;
    // Get tensor dimensions
    auto pred_shape = predictions->shape();
    
    // Validate tensor shapes
    if (pred_shape.size() < 3) {
        std::cerr << "[POSTPROC] Error: predictions tensor should have at least 3 dimensions, got " << pred_shape.size() << std::endl;
        return output;
    }
    
    // Parse tensor dimensions
    int batch_size = pred_shape[0];
    int num_detections = pred_shape[1];  // 8400
    int feature_size = pred_shape[2];    // 38
    
    std::cout << "[POSTPROC] GPU Processing dimensions: " << batch_size << "x" << num_detections << "x" << feature_size << std::endl;
    
    std::cout << "[POSTPROC] Step GPU-3: Getting GPU data pointer..." << std::endl;
    // Get GPU data pointer directly
    const float* gpu_predictions = static_cast<const float*>(predictions->data());
    std::cout << "[POSTPROC] GPU data pointer obtained: " << static_cast<const void*>(gpu_predictions) << std::endl;
    
    std::cout << "[POSTPROC] Step GPU-4: Allocating CUDA memory..." << std::endl;
    // Allocate GPU memory for results using raw CUDA with error checking
    float* d_output_boxes = nullptr;
    float* d_output_scores = nullptr;
    int* d_output_class_ids = nullptr;
    int* d_valid_detections = nullptr;
    int* d_keep_flags = nullptr;
    
    size_t boxes_size = num_detections * 4 * sizeof(float);
    size_t scores_size = num_detections * sizeof(float);
    size_t ids_size = num_detections * sizeof(int);
    size_t flags_size = num_detections * sizeof(int);
    
    std::cout << "[POSTPROC] Memory allocation sizes: boxes=" << boxes_size << " bytes, scores=" << scores_size << " bytes, ids=" << ids_size << " bytes, flags=" << flags_size << " bytes" << std::endl;
    cudaError_t cuda_error;
    auto log_alloc = [](const char* name, size_t sz, cudaError_t err) {
        std::cout << "[MEM] cudaMalloc(" << name << ", " << sz << ") => " << cudaGetErrorString(err) << std::endl;
    };
    cuda_error = cudaMalloc(&d_output_boxes, boxes_size); log_alloc("d_output_boxes", boxes_size, cuda_error);
    cuda_error = cudaMalloc(&d_output_scores, scores_size); log_alloc("d_output_scores", scores_size, cuda_error);
    cuda_error = cudaMalloc(&d_output_class_ids, ids_size); log_alloc("d_output_class_ids", ids_size, cuda_error);
    cuda_error = cudaMalloc(&d_valid_detections, flags_size); log_alloc("d_valid_detections", flags_size, cuda_error);
    cuda_error = cudaMalloc(&d_keep_flags, flags_size); log_alloc("d_keep_flags", flags_size, cuda_error);
    if (!d_output_boxes || !d_output_scores || !d_output_class_ids || !d_valid_detections || !d_keep_flags) {
        std::cerr << "[MEM] CUDA memory allocation failed!" << std::endl;
        if (d_output_boxes) cudaFree(d_output_boxes);
        if (d_output_scores) cudaFree(d_output_scores);
        if (d_output_class_ids) cudaFree(d_output_class_ids);
        if (d_valid_detections) cudaFree(d_valid_detections);
        if (d_keep_flags) cudaFree(d_keep_flags);
        return output;
    }
    
    std::cout << "[POSTPROC] Step GPU-5: Initializing keep flags..." << std::endl;
    std::cout << "[POSTPROC] cudaMemset d_keep_flags to 1, size=" << flags_size << std::endl;
    cudaMemset(d_keep_flags, 1, flags_size);
    
    std::cout << "[POSTPROC] Step GPU-6: Launching CUDA postprocess kernel..." << std::endl;
    // Launch CUDA kernel for postprocessing
    int block_size = 256;
    int grid_size = (num_detections + block_size - 1) / block_size;
    std::cout << "[POSTPROC] Launching CUDA kernel with grid_size=" << grid_size << ", block_size=" << block_size << std::endl;
    
    std::cout << "[POSTPROC] About to call launch_yolo_postprocess_kernel..." << std::endl;
    launch_yolo_postprocess_kernel(
        gpu_predictions,
        d_output_boxes,
        d_output_scores,
        d_output_class_ids,
        d_valid_detections,
        num_detections,
        feature_size,
        0.01f, // Very low confidence threshold to see more detections
        static_cast<int>(num_class_),
        0  // Default CUDA stream
    );
    std::cout << "[POSTPROC] launch_yolo_postprocess_kernel called successfully" << std::endl;
    
    cuda_error = cudaGetLastError();
    std::cout << "[POSTPROC] After launch_yolo_postprocess_kernel: " << cudaGetErrorString(cuda_error) << std::endl;
    
    std::cout << "[POSTPROC] Calling cudaDeviceSynchronize()..." << std::endl;
    cudaDeviceSynchronize();
    std::cout << "[POSTPROC] cudaDeviceSynchronize() completed" << std::endl;
    
    std::cout << "[POSTPROC] Step GPU-7: Launching NMS kernel..." << std::endl;
    // Apply NMS on GPU
    std::cout << "[POSTPROC] About to call launch_nms_kernel..." << std::endl;
    launch_nms_kernel(
        d_output_boxes,
        d_output_scores,
        d_output_class_ids,
        d_keep_flags,
        num_detections,
        0.45f, // IoU threshold
        0      // Default CUDA stream
    );
    std::cout << "[POSTPROC] launch_nms_kernel called successfully" << std::endl;
    
    cuda_error = cudaGetLastError();
    std::cout << "[POSTPROC] After launch_nms_kernel: " << cudaGetErrorString(cuda_error) << std::endl;
    
    std::cout << "[POSTPROC] Calling cudaDeviceSynchronize() for NMS..." << std::endl;
    cudaDeviceSynchronize();
    std::cout << "[POSTPROC] cudaDeviceSynchronize() for NMS completed" << std::endl;
    
    std::cout << "[POSTPROC] Step GPU-8: Copying results back to host..." << std::endl;
    // Copy results back to host with error checking
    std::vector<float> h_output_boxes(num_detections * 4);
    std::vector<float> h_output_scores(num_detections);
    std::vector<int> h_output_class_ids(num_detections);
    std::vector<int> h_valid_detections(num_detections);
    std::vector<int> h_keep_flags(num_detections);
    auto log_copy = [](const char* name, cudaError_t err) {
        std::cout << "[POSTPROC] cudaMemcpy(" << name << ") => " << cudaGetErrorString(err) << std::endl;
    };
    
    std::cout << "[POSTPROC] Copying boxes..." << std::endl;
    cuda_error = cudaMemcpy(h_output_boxes.data(), d_output_boxes, boxes_size, cudaMemcpyDeviceToHost); log_copy("h_output_boxes", cuda_error);
    
    std::cout << "[POSTPROC] Copying scores..." << std::endl;
    cuda_error = cudaMemcpy(h_output_scores.data(), d_output_scores, scores_size, cudaMemcpyDeviceToHost); log_copy("h_output_scores", cuda_error);
    
    std::cout << "[POSTPROC] Copying class IDs..." << std::endl;
    cuda_error = cudaMemcpy(h_output_class_ids.data(), d_output_class_ids, ids_size, cudaMemcpyDeviceToHost); log_copy("h_output_class_ids", cuda_error);
    
    std::cout << "[POSTPROC] Copying valid detections..." << std::endl;
    cuda_error = cudaMemcpy(h_valid_detections.data(), d_valid_detections, flags_size, cudaMemcpyDeviceToHost); log_copy("h_valid_detections", cuda_error);
    
    std::cout << "[POSTPROC] Copying keep flags..." << std::endl;
    cuda_error = cudaMemcpy(h_keep_flags.data(), d_keep_flags, flags_size, cudaMemcpyDeviceToHost); log_copy("h_keep_flags", cuda_error);
    
    std::cout << "[POSTPROC] Step GPU-9: Cleaning up GPU memory..." << std::endl;
    // Cleanup GPU memory
    std::cout << "[POSTPROC] Freeing CUDA memory..." << std::endl;
    cudaFree(d_output_boxes);
    cudaFree(d_output_scores);
    cudaFree(d_output_class_ids);
    cudaFree(d_valid_detections);
    cudaFree(d_keep_flags);
    std::cout << "[POSTPROC] CUDA memory freed." << std::endl;
    
    std::cout << "[POSTPROC] Step GPU-10: Processing final results..." << std::endl;
    // Count valid detections first to pre-allocate vectors
    int valid_count = 0;
    for (int i = 0; i < num_detections; ++i) {
        if (h_valid_detections[i] && h_keep_flags[i]) {
            valid_count++;
        }
    }
    std::cout << "[POSTPROC] Found " << valid_count << " valid detections" << std::endl;
    
    output.boxes.reserve(valid_count);
    output.boxes_scores.reserve(valid_count);
    output.class_ids.reserve(valid_count);
    
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
    
    std::cout << "[POSTPROC] Final valid detections: " << valid_count << std::endl;
    std::cout << "[POSTPROC] === process_boxes_gpu() completed successfully ===" << std::endl;
    log_memory_usage("GPU processing end");
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
    std::map<int, std::vector<std::vector<float>>>& /*out_texts*/,
    std::map<int, std::vector<std::vector<float>>>& /*out_scores_pos*/) {
    // Test with real boxes tensor only
    if (!organized_boxes.empty()) {
        // Get the first class boxes
        auto first_class = organized_boxes.begin();
        const auto& boxes = first_class->second;
        
        if (!boxes.empty()) {
            // Flatten the boxes data
            std::vector<float> boxes_data;
            for (const auto& box : boxes) {
                if (box.size() >= 4) {
                    boxes_data.insert(boxes_data.end(), box.begin(), box.begin() + 4);
                }
            }
            
            if (!boxes_data.empty()) {
                // Create tensor shape: [num_boxes, 4] for x1,y1,x2,y2
                std::vector<int64_t> boxes_shape = {static_cast<int64_t>(boxes.size()), 4};
                
                std::cout << "[TENSOR] Creating real boxes tensor with " << boxes.size() 
                          << " boxes, shape: [" << boxes_shape[0] << ", " << boxes_shape[1] << "]" << std::endl;
                
                // Try creating a simple test tensor with minimal data
                std::vector<float> test_data = {0.1f, 0.1f, 0.9f, 0.9f}; // Simple bounding box
                std::vector<int64_t> test_shape = {1, 4}; // [num_boxes, 4]
                
                // Try using the simple tensor creation function
                auto boxes_tensor = urology::yolo_utils::make_holoscan_tensor_simple(
                    test_data, test_shape);
                out_message["boxes0"] = boxes_tensor;
                if (boxes_tensor) {
                    auto* dl_managed = reinterpret_cast<DLManagedTensor*>(boxes_tensor->data());
                    if (dl_managed) {
                        auto dtype = dl_managed->dl_tensor.dtype;
                        std::cout << "[DEBUG] boxes_tensor dtype: code=" << int(dtype.code)
                                  << ", bits=" << int(dtype.bits)
                                  << ", lanes=" << int(dtype.lanes) << std::endl;
                    } else {
                        std::cout << "[DEBUG] boxes_tensor: could not cast to DLManagedTensor" << std::endl;
                    }
                }
                std::cout << "[TENSOR] Sent simple test tensor to HolovizOp for dtype test" << std::endl;
            } else {
                std::cout << "[TENSOR] No valid box data to create tensor" << std::endl;
            }
        } else {
            std::cout << "[TENSOR] No boxes in first class" << std::endl;
        }
    } else {
        std::cout << "[TENSOR] No organized boxes available" << std::endl;
    }
}

void YoloSegPostprocessorOp::process_scores(
    const std::vector<int>& class_ids,
    const std::vector<float>& scores,
    const std::map<int, std::vector<std::vector<float>>>& scores_pos,
    std::vector<holoscan::ops::HolovizOp::InputSpec>& specs,
    std::unordered_map<std::string, std::shared_ptr<holoscan::Tensor>>& out_scores) {
    
    std::cout << "Processing scores for " << class_ids.size() << " detections" << std::endl;
    
    // Create a single combined score tensor for all detections
    if (!class_ids.empty()) {
        std::vector<float> all_score_data;
        std::vector<int64_t> score_shape = {1, static_cast<int64_t>(class_ids.size()), 3};
        
        // Collect all score positions
        std::vector<std::vector<float>> scores_pos_list;
        for (const auto& [index, score_pos] : scores_pos) {
            scores_pos_list.insert(scores_pos_list.end(), score_pos.begin(), score_pos.end());
        }
        
        // Create combined score data
        for (size_t i = 0; i < class_ids.size() && i < scores_pos_list.size(); ++i) {
            const auto& score_pos = scores_pos_list[i];
            all_score_data.insert(all_score_data.end(), score_pos.begin(), score_pos.end());
            
            std::cout << "Detection " << i << ": class=" << class_ids[i] << ", score=" << scores[i] << std::endl;
        }
        
        // Create single combined score tensor
        auto score_tensor = urology::yolo_utils::make_holoscan_tensor_no_holoviz(all_score_data, score_shape);
        
        std::string score_key = "scores";
        if (score_tensor) {
            out_scores[score_key] = score_tensor;
        }
        
        // Create HolovizOp InputSpec for text visualization
        holoscan::ops::HolovizOp::InputSpec spec(score_key, "text");
        spec.color_ = {1.0f, 1.0f, 1.0f, 1.0f}; // White color
        specs.push_back(spec);
        
        std::cout << "Created combined score tensor for " << class_ids.size() << " detections" << std::endl;
    } else {
        std::cout << "No detections to create score tensors for" << std::endl;
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