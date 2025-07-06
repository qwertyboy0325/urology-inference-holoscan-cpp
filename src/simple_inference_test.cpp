#include "holoscan_fix.hpp"
#include <holoscan/holoscan.hpp>
#include <holoscan/operators/video_stream_replayer/video_stream_replayer.hpp>
#include <holoscan/operators/format_converter/format_converter.hpp>
#include <holoscan/operators/inference/inference.hpp>

using namespace holoscan;

// Simple sink operator to receive inference results
class SinkOp : public holoscan::Operator {
public:
    HOLOSCAN_OPERATOR_FORWARD_ARGS(SinkOp)
    
    SinkOp() = default;
    
    void setup(OperatorSpec& spec) override {
        spec.input<holoscan::gxf::Entity>("in");
    }
    
    void compute(InputContext& op_input, OutputContext&, ExecutionContext&) override {
        auto in_message = op_input.receive<holoscan::gxf::Entity>("in");
        if (in_message) {
            std::cout << "Received inference result!" << std::endl;
        }
    }
};

class SimpleInferenceApp : public holoscan::Application {
public:
    void compose() override {
        using namespace holoscan;
        
        std::cout << "=== Simple Inference Test ===" << std::endl;
        
        // Memory pools (increased size for inference)
        auto host_memory_pool = make_resource<BlockMemoryPool>(
            "host_memory_pool", 
            Arg("storage_type", 0),
            Arg("block_size", 128UL * 1024 * 1024),  // 128MB
            Arg("num_blocks", int64_t(4))
        );
        
        auto device_memory_pool = make_resource<BlockMemoryPool>(
            "device_memory_pool", 
            Arg("storage_type", 1),
            Arg("block_size", 128UL * 1024 * 1024),  // 128MB
            Arg("num_blocks", int64_t(4))
        );
        
        auto cuda_stream_pool = make_resource<CudaStreamPool>(
            "cuda_stream_pool",
            Arg("stream_flags", int64_t(0)),
            Arg("stream_priority", int64_t(0)),
            Arg("reserved_size", int64_t(1)),
            Arg("max_size", int64_t(5))
        );
        
        // Video replayer
        auto replayer = make_operator<ops::VideoStreamReplayerOp>(
            "replayer",
            Arg("directory", std::string("/workspace/data/inputs")),
            Arg("basename", std::string("tensor")),
            Arg("frame_rate", 30.0f),
            Arg("repeat", false),
            Arg("realtime", false),
            Arg("count", int64_t(5))  // Only process 5 frames
        );
        
        // Format converter
        auto format_converter = make_operator<ops::FormatConverterOp>(
            "format_converter",
            Arg("out_tensor_name", std::string("seg_preprocessed")),
            Arg("out_dtype", std::string("float32")),
            Arg("in_dtype", std::string("rgb888")),
            Arg("resize_width", 640),
            Arg("resize_height", 640),
            Arg("pool", device_memory_pool)
        );
        
        // Inference operator
        std::string model_file = "/workspace/data/models/urology_yolov9c_3000random640resize_20240811_4.34_nhwc.onnx";
        
        ops::InferenceOp::DataMap model_path_map;
        model_path_map.insert("seg", model_file);
        
        ops::InferenceOp::DataVecMap pre_processor_map;
        pre_processor_map.insert("seg", std::vector<std::string>{"seg_preprocessed"});
        
        ops::InferenceOp::DataVecMap inference_map;
        inference_map.insert("seg", std::vector<std::string>{"outputs", "proto"});
        
        auto inference = make_operator<ops::InferenceOp>(
            "inference",
            Arg("model_path_map", model_path_map),
            Arg("pre_processor_map", pre_processor_map),
            Arg("inference_map", inference_map),
            Arg("backend", std::string("trt")),
            Arg("enable_fp16", true),
            Arg("allocator", device_memory_pool),
            Arg("cuda_stream_pool", cuda_stream_pool)
        );
        
        // Simple output operator to receive inference results
        auto output_op = make_operator<SinkOp>("output");
        
        // Connect pipeline
        add_flow(replayer, format_converter, {{"output", "source_video"}});
        add_flow(format_converter, inference, {{"tensor", "receivers"}});
        add_flow(inference, output_op, {{"transmitter", "in"}});
        
        std::cout << "Simple inference pipeline setup completed!" << std::endl;
    }
};

int main(int argc, char** argv) {
    auto app = holoscan::make_application<SimpleInferenceApp>();
    app->run();
    return 0;
} 