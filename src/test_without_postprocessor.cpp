#include "holoscan_fix.hpp"
#include <holoscan/operators/video_stream_replayer/video_stream_replayer.hpp>
#include <holoscan/operators/format_converter/format_converter.hpp>
#include <holoscan/operators/inference/inference.hpp>
#include <holoscan/operators/holoviz/holoviz.hpp>
#include <iostream>
#include <filesystem>

class TestWithoutPostprocessorApp : public holoscan::Application {
public:
    TestWithoutPostprocessorApp(const std::string& data_path) : data_path_(data_path) {}

    void compose() override {
        std::cout << "=== Testing Pipeline WITHOUT YoloSegPostprocessorOp ===" << std::endl;
        std::cout << "Data path: " << data_path_ << std::endl;
        
        // Memory pools
        auto host_memory_pool = make_resource<holoscan::BlockMemoryPool>(
            "host_memory_pool", 
            holoscan::Arg("storage_type", 0),
            holoscan::Arg("block_size", 512UL * 1024 * 1024),  // 512MB
            holoscan::Arg("num_blocks", int64_t(16))
        );
        
        auto device_memory_pool = make_resource<holoscan::BlockMemoryPool>(
            "device_memory_pool", 
            holoscan::Arg("storage_type", 1),
            holoscan::Arg("block_size", 1024UL * 1024 * 1024),  // 1GB
            holoscan::Arg("num_blocks", int64_t(16))
        );
        
        auto cuda_stream_pool = make_resource<holoscan::CudaStreamPool>(
            "cuda_stream_pool",
            holoscan::Arg("stream_flags", int64_t(0)),
            holoscan::Arg("stream_priority", int64_t(0)),
            holoscan::Arg("reserved_size", int64_t(1)),
            holoscan::Arg("max_size", int64_t(5))
        );
        
        // Video replayer
        std::cout << "[TEST] Creating video replayer..." << std::endl;
        auto replayer = make_operator<holoscan::ops::VideoStreamReplayerOp>(
            "replayer",
            holoscan::Arg("directory", data_path_ + "/inputs"),
            holoscan::Arg("basename", std::string("tensor")),
            holoscan::Arg("frame_rate", 30.0f),
            holoscan::Arg("repeat", false),
            holoscan::Arg("realtime", false),
            holoscan::Arg("count", int64_t(10))  // Only process 10 frames for testing
        );
        
        // Format converter
        std::cout << "[TEST] Creating format converter..." << std::endl;
        auto format_converter = make_operator<holoscan::ops::FormatConverterOp>(
            "format_converter",
            holoscan::Arg("out_tensor_name", std::string("seg_preprocessed")),
            holoscan::Arg("out_dtype", std::string("float32")),
            holoscan::Arg("in_dtype", std::string("rgb888")),
            holoscan::Arg("resize_width", 640),
            holoscan::Arg("resize_height", 640),
            holoscan::Arg("pool", device_memory_pool)
        );
        
        // Inference operator (without postprocessor)
        std::cout << "[TEST] Creating inference operator..." << std::endl;
        std::string model_file = data_path_ + "/models/urology_yolov9c_3000random640resize_20240811_4.34_nhwc.onnx";
        
        if (!std::filesystem::exists(model_file)) {
            throw std::runtime_error("Model file not found: " + model_file);
        }
        
        holoscan::ops::InferenceOp::DataMap model_path_map;
        model_path_map.insert("seg", model_file);
        
        holoscan::ops::InferenceOp::DataVecMap pre_processor_map;
        pre_processor_map.insert("seg", std::vector<std::string>{"seg_preprocessed"});
        
        holoscan::ops::InferenceOp::DataVecMap inference_map;
        inference_map.insert("seg", std::vector<std::string>{"outputs", "proto"});
        
        auto inference = make_operator<holoscan::ops::InferenceOp>(
            "inference",
            holoscan::Arg("model_path_map", model_path_map),
            holoscan::Arg("pre_processor_map", pre_processor_map),
            holoscan::Arg("inference_map", inference_map),
            holoscan::Arg("backend", std::string("trt")),
            holoscan::Arg("enable_fp16", true),
            holoscan::Arg("allocator", device_memory_pool),
            holoscan::Arg("cuda_stream_pool", cuda_stream_pool)
        );
        
        // Simple visualizer (no postprocessor)
        std::cout << "[TEST] Creating visualizer..." << std::endl;
        auto visualizer = make_operator<holoscan::ops::HolovizOp>(
            "visualizer",
            holoscan::Arg("width", int64_t(1920)),
            holoscan::Arg("height", int64_t(1080)),
            holoscan::Arg("enable_render_buffer_input", false),
            holoscan::Arg("enable_render_buffer_output", false),
            holoscan::Arg("allocator", host_memory_pool),
            holoscan::Arg("cuda_stream_pool", cuda_stream_pool)
        );
        
        // Connect pipeline WITHOUT postprocessor
        std::cout << "[TEST] Connecting pipeline (without postprocessor)..." << std::endl;
        add_flow(replayer, visualizer, {{"output", "receivers"}});                // Video stream directly to visualizer
        add_flow(replayer, format_converter, {{"output", "source_video"}});       // Video stream to preprocessor
        add_flow(format_converter, inference, {{"tensor", "receivers"}});         // Preprocessed tensor to inference
        add_flow(inference, visualizer, {{"transmitter", "receivers"}});          // Inference results directly to visualizer
        
        std::cout << "[TEST] Pipeline setup completed (without postprocessor)" << std::endl;
    }

private:
    std::string data_path_;
};

int main(int argc, char** argv) {
    std::cout << "=== Testing Pipeline WITHOUT YoloSegPostprocessorOp ===" << std::endl;
    
    std::string data_path = "./data";
    if (argc > 1) {
        data_path = argv[1];
    }
    
    std::cout << "Data path: " << data_path << std::endl;
    
    try {
        TestWithoutPostprocessorApp app(data_path);
        app.run();
        std::cout << "[TEST] SUCCESS: Pipeline completed without postprocessor!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[TEST] FAILED: " << e.what() << std::endl;
        return 1;
    }
} 