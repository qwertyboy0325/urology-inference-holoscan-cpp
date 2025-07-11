#include <cuda_runtime.h>

// CUDA kernel for GPU-based YOLO postprocessing
__global__ void yolo_postprocess_kernel(
    const float* predictions,
    float* output_boxes,
    float* output_scores,
    int* output_class_ids,
    int* valid_detections,
    int num_detections,
    int feature_size,
    float confidence_threshold,
    int num_classes
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx >= num_detections) return;
    
    // Calculate offset for this detection
    int offset = idx * feature_size;
    
    // Extract basic features (using coalesced memory access)
    float x = predictions[offset + 0];
    float y = predictions[offset + 1];
    float w = predictions[offset + 2];
    float h = predictions[offset + 3];
    float confidence = predictions[offset + 4];
    float class_id_float = predictions[offset + 5];
    
    // Early exit for invalid detections
    int class_id = static_cast<int>(class_id_float);
    if (class_id < 0 || class_id >= num_classes || confidence < confidence_threshold) {
        valid_detections[idx] = 0;
        return;
    }
    
    // Convert to xyxy format (clamp to valid range)
    float x1 = fmaxf(0.0f, x - w / 2.0f);
    float y1 = fmaxf(0.0f, y - h / 2.0f);
    float x2 = fminf(1.0f, x + w / 2.0f);
    float y2 = fminf(1.0f, y + h / 2.0f);
    
    // Validate box dimensions
    if (x2 <= x1 || y2 <= y1) {
        valid_detections[idx] = 0;
        return;
    }
    
    // Store results
    output_boxes[idx * 4 + 0] = x1;
    output_boxes[idx * 4 + 1] = y1;
    output_boxes[idx * 4 + 2] = x2;
    output_boxes[idx * 4 + 3] = y2;
    output_scores[idx] = confidence;
    output_class_ids[idx] = class_id;
    valid_detections[idx] = 1;
}

// CUDA kernel for Non-Maximum Suppression (optimized)
__global__ void nms_kernel(
    const float* boxes,
    const float* scores,
    const int* class_ids,
    int* keep_flags,
    int num_detections,
    float iou_threshold
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx >= num_detections) return;
    
    if (keep_flags[idx] == 0) return; // Already suppressed
    
    float x1_i = boxes[idx * 4 + 0];
    float y1_i = boxes[idx * 4 + 1];
    float x2_i = boxes[idx * 4 + 2];
    float y2_i = boxes[idx * 4 + 3];
    float score_i = scores[idx];
    int class_i = class_ids[idx];
    
    // Calculate area_i once
    float area_i = (x2_i - x1_i) * (y2_i - y1_i);
    
    // Compare with all other detections
    for (int j = idx + 1; j < num_detections; ++j) {
        if (keep_flags[j] == 0) continue; // Already suppressed
        if (class_ids[j] != class_i) continue; // Different class
        
        float x1_j = boxes[j * 4 + 0];
        float y1_j = boxes[j * 4 + 1];
        float x2_j = boxes[j * 4 + 2];
        float y2_j = boxes[j * 4 + 3];
        float score_j = scores[j];
        
        // Calculate IoU (optimized)
        float x1_inter = fmaxf(x1_i, x1_j);
        float y1_inter = fmaxf(y1_i, y1_j);
        float x2_inter = fminf(x2_i, x2_j);
        float y2_inter = fminf(y2_i, y2_j);
        
        // Early exit if no overlap
        if (x2_inter <= x1_inter || y2_inter <= y1_inter) continue;
        
        float inter_area = (x2_inter - x1_inter) * (y2_inter - y1_inter);
        float area_j = (x2_j - x1_j) * (y2_j - y1_j);
        float union_area = area_i + area_j - inter_area;
        
        // Avoid division by zero
        if (union_area <= 0.0f) continue;
        
        float iou = inter_area / union_area;
        
        // Suppress the detection with lower score
        if (iou > iou_threshold) {
            if (score_i > score_j) {
                keep_flags[j] = 0;
            } else {
                keep_flags[idx] = 0;
                break;
            }
        }
    }
}

// C wrapper functions for calling from C++ code
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
        cudaStream_t stream = 0
    ) {
        int block_size = 256;
        int grid_size = (num_detections + block_size - 1) / block_size;
        
        yolo_postprocess_kernel<<<grid_size, block_size, 0, stream>>>(
            predictions,
            output_boxes,
            output_scores,
            output_class_ids,
            valid_detections,
            num_detections,
            feature_size,
            confidence_threshold,
            num_classes
        );
    }
    
    void launch_nms_kernel(
        const float* boxes,
        const float* scores,
        const int* class_ids,
        int* keep_flags,
        int num_detections,
        float iou_threshold,
        cudaStream_t stream = 0
    ) {
        int block_size = 256;
        int grid_size = (num_detections + block_size - 1) / block_size;
        
        nms_kernel<<<grid_size, block_size, 0, stream>>>(
            boxes,
            scores,
            class_ids,
            keep_flags,
            num_detections,
            iou_threshold
        );
    }
} 