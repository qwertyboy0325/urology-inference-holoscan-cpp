#include "operators/yolo_seg_postprocessor.hpp"
#include "utils/yolo_utils.hpp"

#include <holoscan/holoscan.hpp>
#include <cuda_runtime.h>
#include <iostream>
#include <algorithm>
#include <cmath>

namespace urology {

YoloSegPostprocessorOp::YoloSegPostprocessorOp(const std::map<int, LabelInfo>& label_dict)
    : label_dict_(label_dict) {}

void YoloSegPostprocessorOp::setup(holoscan::OperatorSpec& spec) {
    spec.input("in");
    spec.output("out");
    spec.output("output_specs");
    
    spec.param("scores_threshold", scores_threshold_, "Scores threshold", 
               "Minimum confidence score for detections", 0.2);
    spec.param("num_class", num_class_, "Number of classes", 
               "Number of object classes", 12);
    spec.param("out_tensor_name", out_tensor_name_, "Output tensor name", 
               "Name of the output tensor", std::string("out_tensor"));
}

void YoloSegPostprocessorOp::compute(holoscan::InputContext& op_input, 
                                    holoscan::OutputContext& op_output,
                                    holoscan::ExecutionContext& context) {
    try {
        // Receive input tensors
        std::shared_ptr<holoscan::Tensor> predictions, masks_seg;
        receive_inputs(op_input, predictions, masks_seg);
        
        // Process the predictions
        YoloOutput output = process_boxes(predictions, masks_seg);
        
        // Sort by class IDs
        output.sort_by_class_ids();
        
        // Organize boxes by class
        auto out_boxes = organize_boxes(output.class_ids, output.boxes);
        
        // Prepare output message
        std::unordered_map<std::string, std::shared_ptr<holoscan::Tensor>> out_message;
        std::map<int, std::vector<std::vector<float>>> out_texts, out_scores_pos;
        
        prepare_output_message(out_boxes, out_message, out_texts, out_scores_pos);
        
        // Process scores for visualization
        std::vector<holoscan::ops::HolovizOp::InputSpec> specs;
        std::unordered_map<std::string, std::shared_ptr<holoscan::Tensor>> out_scores;
        
        process_scores(output.class_ids, output.boxes_scores, out_scores_pos, specs, out_scores);
        
        // Add scores to output message
        for (const auto& [key, value] : out_scores) {
            out_message[key] = value;
        }
        
        // Create mask tensor
        if (!output.output_masks.empty()) {
            // Convert masks to tensor format
            std::vector<float> mask_data;
            for (const auto& mask : output.output_masks) {
                mask_data.insert(mask_data.end(), mask.begin(), mask.end());
            }
            
            // Create tensor with appropriate shape
            holoscan::TensorSpec mask_spec("masks", holoscan::nvidia::gxf::PrimitiveType::kFloat32);
            auto mask_tensor = std::make_shared<holoscan::Tensor>(mask_spec);
            // TODO: Set tensor data properly
            out_message["masks"] = mask_tensor;
        }
        
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
    
    if (in_message.find("input_0") != in_message.end()) {
        predictions = in_message["input_0"];
    } else {
        throw std::runtime_error("Missing input_0 tensor");
    }
    
    if (in_message.find("input_1") != in_message.end()) {
        masks_seg = in_message["input_1"];
    } else {
        throw std::runtime_error("Missing input_1 tensor");
    }
}

YoloOutput YoloSegPostprocessorOp::process_boxes(const std::shared_ptr<holoscan::Tensor>& predictions,
                                                 const std::shared_ptr<holoscan::Tensor>& masks_seg) {
    YoloOutput output;
    
    // Get tensor data
    const float* pred_data = static_cast<const float*>(predictions->data());
    const float* mask_data = static_cast<const float*>(masks_seg->data());
    
    // Get tensor dimensions
    auto pred_shape = predictions->shape();
    auto mask_shape = masks_seg->shape();
    
    // Process YOLO detections
    // This is a simplified implementation - you'll need to adapt based on your YOLO model output format
    int num_detections = pred_shape[1]; // Assuming [batch, num_boxes, 5+num_classes]
    int box_size = pred_shape[2];
    
    for (int i = 0; i < num_detections; ++i) {
        const float* box_data = pred_data + i * box_size;
        
        // Extract box coordinates (x, y, w, h)
        float x = box_data[0];
        float y = box_data[1];
        float w = box_data[2];
        float h = box_data[3];
        float conf = box_data[4];
        
        // Check confidence threshold
        if (conf < scores_threshold_) continue;
        
        // Find best class
        int best_class = 0;
        float best_score = box_data[5];
        for (int c = 1; c < num_class_; ++c) {
            if (box_data[5 + c] > best_score) {
                best_score = box_data[5 + c];
                best_class = c;
            }
        }
        
        // Final confidence score
        float final_score = conf * best_score;
        if (final_score < scores_threshold_) continue;
        
        // Convert to corner format
        float x1 = x - w / 2.0f;
        float y1 = y - h / 2.0f;
        float x2 = x + w / 2.0f;
        float y2 = y + h / 2.0f;
        
        output.class_ids.push_back(best_class);
        output.boxes_scores.push_back(final_score);
        output.boxes.push_back({x1, y1, x2, y2});
        
        // Extract corresponding mask (simplified)
        // You'll need to implement proper mask extraction based on your model
        std::vector<float> mask_pixels; // Placeholder
        output.output_masks.push_back(mask_pixels);
    }
    
    return output;
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
    const std::map<int, std::vector<std::vector<float>>>& out_boxes,
    std::unordered_map<std::string, std::shared_ptr<holoscan::Tensor>>& out_message,
    std::map<int, std::vector<std::vector<float>>>& out_texts,
    std::map<int, std::vector<std::vector<float>>>& out_scores_pos) {
    
    for (const auto& [class_id, label] : label_dict_) {
        std::string box_key = "boxes" + std::to_string(class_id);
        std::string label_key = "label" + std::to_string(class_id);
        
        if (out_boxes.find(class_id) != out_boxes.end()) {
            const auto& boxes = out_boxes.at(class_id);
            
            // Create box tensor
            std::vector<float> box_data;
            std::vector<std::vector<float>> text_coords, score_coords;
            
            for (const auto& box : boxes) {
                // Box format: [x1, y1, x2, y2]
                box_data.insert(box_data.end(), box.begin(), box.end());
                
                // Text position (top-left)
                text_coords.push_back({box[0], box[1]});
                
                // Score position (top-right)
                score_coords.push_back({box[2], box[1]});
            }
            
            // Create tensor for boxes
            holoscan::TensorSpec box_spec(box_key, holoscan::nvidia::gxf::PrimitiveType::kFloat32);
            auto box_tensor = std::make_shared<holoscan::Tensor>(box_spec);
            // TODO: Set tensor data properly
            out_message[box_key] = box_tensor;
            
            // Store text and score positions
            out_texts[class_id] = append_size_to_text_coord(text_coords, 0.04f);
            out_scores_pos[class_id] = append_size_to_score_coord(score_coords, 0.04f);
            
        } else {
            // Empty tensors for classes with no detections
            holoscan::TensorSpec empty_spec(box_key, holoscan::nvidia::gxf::PrimitiveType::kFloat32);
            auto empty_tensor = std::make_shared<holoscan::Tensor>(empty_spec);
            out_message[box_key] = empty_tensor;
            
            out_texts[class_id] = {{-1.0f, -1.0f}};
        }
        
        // Create label tensor
        holoscan::TensorSpec label_spec(label_key, holoscan::nvidia::gxf::PrimitiveType::kFloat32);
        auto label_tensor = std::make_shared<holoscan::Tensor>(label_spec);
        // TODO: Set tensor data properly
        out_message[label_key] = label_tensor;
    }
}

void YoloSegPostprocessorOp::process_scores(
    const std::vector<int>& class_ids,
    const std::vector<float>& scores,
    const std::map<int, std::vector<std::vector<float>>>& scores_pos,
    std::vector<holoscan::ops::HolovizOp::InputSpec>& specs,
    std::unordered_map<std::string, std::shared_ptr<holoscan::Tensor>>& out_scores) {
    
    int count = 0;
    for (size_t i = 0; i < class_ids.size(); ++i) {
        int class_id = class_ids[i];
        float score = scores[i];
        
        if (label_dict_.find(class_id) != label_dict_.end()) {
            const auto& label = label_dict_.at(class_id);
            
            // Create visualization spec
            holoscan::ops::HolovizOp::InputSpec spec;
            spec.tensor_name_ = "score" + std::to_string(count);
            spec.type_ = holoscan::ops::HolovizOp::InputType::TEXT;
            spec.color_ = label.color;
            spec.text_ = {std::to_string(score).substr(0, 5)}; // Format score to 3 decimal places
            
            specs.push_back(spec);
            
            // Create score tensor
            holoscan::TensorSpec score_spec("score" + std::to_string(count), 
                                          holoscan::nvidia::gxf::PrimitiveType::kFloat32);
            auto score_tensor = std::make_shared<holoscan::Tensor>(score_spec);
            // TODO: Set tensor data properly
            out_scores["score" + std::to_string(count)] = score_tensor;
            
            count++;
        }
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