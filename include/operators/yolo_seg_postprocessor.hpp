#pragma once

#include <holoscan/holoscan.hpp>
#include <holoscan/operators/holoviz/holoviz.hpp>
#include <cuda_runtime.h>
#include <vector>
#include <map>
#include <string>

namespace urology {

struct LabelInfo {
    std::string text;
    std::vector<float> color;
};

struct YoloOutput {
    std::vector<int> class_ids;
    std::vector<float> boxes_scores;
    std::vector<std::vector<float>> boxes;
    std::vector<std::vector<float>> output_masks;
    
    void sort_by_class_ids();
};

class YoloSegPostprocessorOp : public holoscan::Operator {
public:
    HOLOSCAN_OPERATOR_FORWARD_ARGS(YoloSegPostprocessorOp)

    YoloSegPostprocessorOp() = default;
    YoloSegPostprocessorOp(const std::map<int, LabelInfo>& label_dict);

    void setup(holoscan::OperatorSpec& spec) override;
    void compute(holoscan::InputContext& op_input, 
                holoscan::OutputContext& op_output,
                holoscan::ExecutionContext& context) override;

private:
    void receive_inputs(holoscan::InputContext& op_input,
                       std::shared_ptr<holoscan::Tensor>& predictions,
                       std::shared_ptr<holoscan::Tensor>& masks_seg);
    
    YoloOutput process_boxes(const std::shared_ptr<holoscan::Tensor>& predictions,
                            const std::shared_ptr<holoscan::Tensor>& masks_seg);
    
    std::map<int, std::vector<std::vector<float>>> organize_boxes(
        const std::vector<int>& class_ids,
        const std::vector<std::vector<float>>& boxes);
    
    void prepare_output_message(const std::map<int, std::vector<std::vector<float>>>& out_boxes,
                               std::unordered_map<std::string, std::shared_ptr<holoscan::Tensor>>& out_message,
                               std::map<int, std::vector<std::vector<float>>>& out_texts,
                               std::map<int, std::vector<std::vector<float>>>& out_scores_pos);
    
    void process_scores(const std::vector<int>& class_ids,
                       const std::vector<float>& scores,
                       const std::map<int, std::vector<std::vector<float>>>& scores_pos,
                       std::vector<holoscan::ops::HolovizOp::InputSpec>& specs,
                       std::unordered_map<std::string, std::shared_ptr<holoscan::Tensor>>& out_scores);

    std::map<int, LabelInfo> label_dict_;
    float scores_threshold_;
    int num_class_;
    std::string out_tensor_name_;
};

// Utility functions
std::vector<std::vector<float>> append_size_to_text_coord(
    const std::vector<std::vector<float>>& coords, float size);

std::vector<std::vector<float>> append_size_to_score_coord(
    const std::vector<std::vector<float>>& coords, float size);

} // namespace urology 