#pragma once

#include <holoscan/holoscan.hpp>
#include <holoscan/core/gxf/gxf_component.hpp>
#include <holoscan/core/condition.hpp>
#include <holoscan/operators/holoviz/holoviz.hpp>
#include <gxf/std/tensor.hpp>
#include "urology_app.hpp"

#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <string>

namespace urology {

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

    YoloSegPostprocessorOp();
    virtual ~YoloSegPostprocessorOp();

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
    
    YoloOutput process_boxes_gpu(const std::shared_ptr<holoscan::Tensor>& predictions,
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
    
    void create_placeholder_mask_tensor(
        std::unordered_map<std::string, std::shared_ptr<holoscan::Tensor>>& out_message);

    holoscan::Parameter<double> scores_threshold_;
    holoscan::Parameter<int64_t> num_class_;
    holoscan::Parameter<std::string> out_tensor_name_;
};

// Utility functions
std::vector<std::vector<float>> append_size_to_text_coord(
    const std::vector<std::vector<float>>& coords, float size);

std::vector<std::vector<float>> append_size_to_score_coord(
    const std::vector<std::vector<float>>& coords, float size);

} // namespace urology 