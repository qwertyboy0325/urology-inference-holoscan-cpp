#include "holoscan_fix.hpp"
#include <holoscan/operators/video_stream_replayer/video_stream_replayer.hpp>
#include <holoscan/operators/format_converter/format_converter.hpp>
#include <holoscan/operators/inference/inference.hpp>
#include "operators/yolo_seg_postprocessor.hpp"
#include <iostream>
#include <filesystem>

class TestPostprocessorOnlyApp : public holoscan::Application {
public:
    TestPostprocessorOnlyApp(const std::string& data_path) : data_path_(data_path) {}

    void compose() override {
        std::cout << "=== Testing Postprocessor Only (No HolovizOp) ===" << std::endl;
        std::cout << "Data path: " << data_path_ << std::endl;
        
        // Minimal memory pools
        auto host_memory_pool = make_resource<holoscan::BlockMemoryPool>(
            "host_memory_pool", 
            holoscan::Arg("storage_type", 0),
            holoscan::Arg("block_size", 64UL * 1024 * 1024),   // 64MB
            holoscan::Arg("num_blocks", int64_t(4))
        );
        
        auto device_memory_pool = make_resource<holoscan::BlockMemoryPool>(
            "device_memory_pool", 
            holoscan::Arg("storage_type", 1),
            holoscan::Arg("block_size", 128UL * 1024 * 1024),  // 128MB
            holoscan::Arg("num_blocks", int64_t(4))
        );
        
        auto cuda_stream_pool = make_resource<holoscan::CudaStreamPool>(
            "cuda_stream_pool",
            holoscan::Arg("stream_flags", int64_t(0)),
            holoscan::Arg("stream_priority", int64_t(0)),
            holoscan::Arg("reserved_size", int64_t(1)),
            holoscan::Arg("max_size", int64_t(5))
        );
        
        // Set minimal GXF memory pools
        setenv("GXF_MEMORY_POOL_SIZE", "268435456", 1);   // 256MB
        setenv("HOLOSCAN_MEMORY_POOL_SIZE", "536870912", 1);  // 512MB
        setenv("CUDA_MEMORY_POOL_SIZE", "268435456", 1);  // 256MB
        
        // Video replayer
        std::cout << "[TEST] Creating video replayer..." << std::endl;
        auto replayer = make_operator<holoscan::ops::VideoStreamReplayerOp>(
            "replayer",
            holoscan::Arg("directory", data_path_ + "/inputs"),
            holoscan::Arg("basename", std::string("tensor")),
            holoscan::Arg("frame_rate", 30.0f),
            holoscan::Arg("repeat", false),
            holoscan::Arg("realtime", false),
            holoscan::Arg("count", int64_t(1))  // Only 1 frame
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
        
        // Inference operator
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
        
        // YOLO postprocessor
        std::cout << "[TEST] Creating YOLO postprocessor..." << std::endl;
        auto postprocessor = make_operator<urology::YoloSegPostprocessorOp>(
            "yolo_seg_postprocessor",
            holoscan::Arg("scores_threshold", 0.2),
            holoscan::Arg("num_class", int64_t(12)),
            holoscan::Arg("out_tensor_name", std::string("out_tensor"))
        );
        
        // Simple output operator (no HolovizOp)
        std::cout << "[TEST] Creating simple output operator..." << std::endl;
        auto output = make_operator<holoscan::ops::GXFCodeletOp>("output");
        
        // Connect pipeline (no HolovizOp)
        std::cout << "[TEST] Connecting pipeline (postprocessor only)..." << std::endl;
        add_flow(replayer, format_converter, {{"output", "source_video"}});
        add_flow(format_converter, inference, {{"tensor", "receivers"}});
        add_flow(inference, postprocessor, {{"transmitter", "in"}});
        add_flow(postprocessor, output, {{"out", "in"}});
        
        std::cout << "[TEST] Pipeline setup completed (postprocessor only)" << std::endl;
    }

private:
    std::string data_path_;
};

int main(int argc, char** argv) {
    std::cout << "=== Testing Postprocessor Only (No HolovizOp) ===" << std::endl;
    
    std::string data_path = "./data";
    if (argc > 1) {
        data_path = argv[1];
    }
    
    std::cout << "Data path: " << data_path << std::endl;
    
    try {
        TestPostprocessorOnlyApp app(data_path);
        app.run();
        std::cout << "[TEST] SUCCESS: Postprocessor test completed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[TEST] FAILED: " << e.what() << std::endl;
        return 1;
    }
} 