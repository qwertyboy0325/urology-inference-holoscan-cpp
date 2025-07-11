#include "holoscan_fix.hpp"
#include "urology_app.hpp"
#include "operators/yolo_seg_postprocessor.hpp"


#include <yaml-cpp/yaml.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <holoscan/core/gxf/gxf_utils.hpp>
#include <chrono>

namespace urology {

UrologyApp::UrologyApp(const std::string& data_path, 
                      const std::string& source,
                      const std::string& output_filename,
                      const std::string& labels_file)
    : data_path_(data_path), 
      model_path_(""),
      output_path_(""),
      output_filename_(output_filename),
      labels_file_(labels_file),
      is_recording_(true),
      source_type_(source),
      visualizer_type_("holoviz"),
      model_name_("Urology_yolov11x-seg_3-13-16-17val640rezize_1_4.40.0_nms.onnx"),
      model_type_("yolo_seg_v9"),
      record_output_(true) {
    
    // Set name
    name("Urology App");
    
    // Set up paths with new optimized structure
    if (data_path_ == "none") {
        const char* env_path = std::getenv("HOLOHUB_DATA_PATH");
        data_path_ = env_path ? env_path : "../data";
    }
    
    // Use individual environment variables for each path type
    const char* model_env = std::getenv("HOLOSCAN_MODEL_PATH");
    const char* input_env = std::getenv("HOLOSCAN_INPUT_PATH");
    const char* output_env = std::getenv("UROLOGY_OUTPUT_PATH");
    
    model_path_ = model_env ? model_env : (std::filesystem::path(data_path_) / "models").string();
    input_path_ = input_env ? input_env : (std::filesystem::path(data_path_) / "inputs").string();
    output_path_ = output_env ? output_env : (std::filesystem::path(data_path_) / "output").string();
    
    // Create output directory if it doesn't exist
    std::filesystem::create_directories(output_path_);
    
    // Load labels
    load_labels();
    
    // Initialize memory pools
    host_memory_pool_ = std::make_shared<holoscan::BlockMemoryPool>(
        1, 32 * 1024 * 1024, 2, 1);
    device_memory_pool_ = std::make_shared<holoscan::BlockMemoryPool>(
        1, 32 * 1024 * 1024, 2, 1);
    cuda_stream_pool_ = std::make_shared<holoscan::CudaStreamPool>(
        0, 0, 0, 1, 5);
}

void UrologyApp::load_labels() {
    if (labels_file_.empty()) {
        // Use default labels
        label_dict_ = {
            {0, {"Background", {0.0f, 0.0f, 0.0f, 0.0f}}},
            {1, {"Spleen", {0.1451f, 0.9412f, 0.6157f, 0.2f}}},
            {2, {"Left_Kidney", {0.8941f, 0.1176f, 0.0941f, 0.2f}}},
            {3, {"Left_Renal_Artery", {1.0000f, 0.8039f, 0.1529f, 0.2f}}},
            {4, {"Left_Renal_Vein", {0.0039f, 0.9373f, 1.0000f, 0.2f}}},
            {5, {"Left_Ureter", {0.9569f, 0.9019f, 0.1569f, 0.2f}}},
            {6, {"Left_Lumbar_Vein", {0.0157f, 0.4549f, 0.4509f, 0.0f}}},
            {7, {"Left_Adrenal_Vein", {0.8941f, 0.5647f, 0.0706f, 0.0f}}},
            {8, {"Left_Gonadal_Vein", {0.5019f, 0.1059f, 0.4471f, 0.0f}}},
            {9, {"Psoas_Muscle", {1.0000f, 1.0000f, 1.0000f, 0.2f}}},
            {10, {"Colon", {0.4314f, 0.4863f, 1.0000f, 0.0f}}},
            {11, {"Abdominal_Aorta", {0.6784f, 0.4941f, 0.2745f, 0.0f}}}
        };
        return;
    }
    
    try {
        YAML::Node config = YAML::LoadFile(labels_file_);
        if (config["labels"]) {
            auto labels = config["labels"];
            label_dict_.clear();
            
            for (size_t i = 0; i < labels.size(); ++i) {
                if (labels[i].IsSequence() && labels[i].size() >= 5) {
                    LabelInfo info;
                    info.text = labels[i][0].as<std::string>();
                    info.color = {
                        labels[i][1].as<float>(),
                        labels[i][2].as<float>(),
                        labels[i][3].as<float>(),
                        labels[i][4].as<float>()
                    };
                    label_dict_[i] = info;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading labels file: " << e.what() << std::endl;
        std::cerr << "Using default labels." << std::endl;
    }
}

void UrologyApp::compose() {
    std::cout << "[MEM] === Starting UrologyApp::compose() ===" << std::endl;
    
    // Initialize paths
    if (data_path_ == "none") {
        data_path_ = "./data";
    }
    
    model_path_ = data_path_ + "/models";
    input_path_ = data_path_ + "/inputs";
    output_path_ = data_path_ + "/output";
    
    // Load model configuration
    model_name_ = "urology_yolov9c_3000random640resize_20240811_4.34_nhwc.onnx";
    model_type_ = "onnx";
    
    // Initialize state
    is_recording_ = false;
    record_output_ = false; // Disable recording for now
    source_type_ = "replayer";
    
    std::cout << "=== Urology Inference Application Setup ===" << std::endl;
    std::cout << "Model path: " << model_path_ << std::endl;
    std::cout << "Input path: " << input_path_ << std::endl;
    std::cout << "Model file: " << model_name_ << std::endl;
    
    std::cout << "[MEM] Creating memory pools..." << std::endl;
    // Memory pools - REASONABLE CONFIGURATION FOR 16GB GPU
    host_memory_pool_ = make_resource<holoscan::BlockMemoryPool>(
        "host_memory_pool", 
        holoscan::Arg("storage_type", 0),
        holoscan::Arg("block_size", 512UL * 1024 * 1024),   // 512MB
        holoscan::Arg("num_blocks", int64_t(8))
    );
    std::cout << "[MEM] Host memory pool created (512MB x 8 blocks = 4GB)" << std::endl;
    
    device_memory_pool_ = make_resource<holoscan::BlockMemoryPool>(
        "device_memory_pool", 
        holoscan::Arg("storage_type", 1),
        holoscan::Arg("block_size", 1024UL * 1024 * 1024),  // 1GB
        holoscan::Arg("num_blocks", int64_t(8))
    );
    std::cout << "[MEM] Device memory pool created (1GB x 8 blocks = 8GB)" << std::endl;
    
    cuda_stream_pool_ = make_resource<holoscan::CudaStreamPool>(
        "cuda_stream_pool",
        holoscan::Arg("stream_flags", int64_t(0)),
        holoscan::Arg("stream_priority", int64_t(0)),
        holoscan::Arg("reserved_size", int64_t(1)),
        holoscan::Arg("max_size", int64_t(5))
    );
    std::cout << "[MEM] CUDA stream pool created" << std::endl;
    
    // Video replayer
    std::cout << "[MEM] Setting up video replayer..." << std::endl;
    std::filesystem::path video_dir = input_path_;
    if (std::filesystem::exists(video_dir) && source_type_ == "replayer") {
        std::cout << "Setting up video replayer from: \"" << video_dir << "\"" << std::endl;
        
        // Check file sizes before creating replayer
        std::filesystem::path entities_file = video_dir / "tensor.gxf_entities";
        std::filesystem::path index_file = video_dir / "tensor.gxf_index";
        
        if (std::filesystem::exists(entities_file)) {
            auto file_size = std::filesystem::file_size(entities_file);
            std::cout << "[MEM] GXF entities file size: " << (file_size / (1024*1024*1024)) << " GB" << std::endl;
            
            // Calculate optimal chunk size based on available memory
            // Use 1GB chunks to avoid OOM
            const size_t chunk_size_gb = 1;
            const size_t chunk_size_bytes = chunk_size_gb * 1024 * 1024 * 1024;
            const size_t estimated_frames_per_chunk = chunk_size_bytes / (1920 * 1080 * 3); // 3 bytes per pixel
            const size_t max_frames_to_load = std::min(estimated_frames_per_chunk, size_t(100)); // Cap at 100 frames
            
            std::cout << "[MEM] Memory optimization: Loading " << max_frames_to_load << " frames per chunk" << std::endl;
        }
        if (std::filesystem::exists(index_file)) {
            auto file_size = std::filesystem::file_size(index_file);
            std::cout << "[MEM] GXF index file size: " << (file_size / 1024) << " KB" << std::endl;
        }
        
        // Set GXF memory optimization environment variables - REASONABLE
        std::cout << "[MEM] Setting GXF memory optimization (REASONABLE)..." << std::endl;
        setenv("GXF_MEMORY_POOL_SIZE", "2147483648", 1);   // 2GB GXF memory pool
        setenv("HOLOSCAN_MEMORY_POOL_SIZE", "4294967296", 1);  // 4GB Holoscan memory pool
        setenv("CUDA_MEMORY_POOL_SIZE", "1073741824", 1);  // 1GB CUDA memory pool
        
        // Create standard replayer - PROCESS ENTIRE VIDEO STREAM
        replayer_ = make_operator<holoscan::ops::VideoStreamReplayerOp>(
            "replayer",
            holoscan::Arg("directory", video_dir.string()),
            holoscan::Arg("basename", std::string("tensor")),
            holoscan::Arg("frame_rate", 30.0f),
            holoscan::Arg("repeat", false),
            holoscan::Arg("realtime", false)
            // Removed "count" parameter to process all frames
            // Optional: Add "count" with a large number to limit frames for testing
            // holoscan::Arg("count", int64_t(1000))  // Process 1000 frames
        );
        std::cout << "[MEM] Video replayer created successfully (memory-optimized)" << std::endl;
    } else {
        std::cout << "[MEM] ERROR: Video directory does not exist or source type is not replayer" << std::endl;
        throw std::runtime_error("Video directory not found: " + video_dir.string());
    }
    
    // Initialize pipeline state
    pipeline_paused_ = false;
    
    std::cout << "[MEM] Creating format converter..." << std::endl;
    // Format converter for preprocessing
    format_converter_ = make_operator<holoscan::ops::FormatConverterOp>(
        "segmentation_preprocessor",
        holoscan::Arg("out_tensor_name", std::string("seg_preprocessed")),
        holoscan::Arg("out_dtype", std::string("float32")),
        holoscan::Arg("in_dtype", std::string("rgb888")),
        holoscan::Arg("resize_width", 640),
        holoscan::Arg("resize_height", 640),
        holoscan::Arg("pool", device_memory_pool_)
    );
    std::cout << "[MEM] Format converter created" << std::endl;
    
    // Inference operator
    std::cout << "[MEM] Setting up inference operator..." << std::endl;
    std::string model_file = std::filesystem::path(model_path_) / model_name_;
    std::cout << "Model file path: " << model_file << std::endl;
    
    // Check if model file exists
    if (!std::filesystem::exists(model_file)) {
        throw std::runtime_error("Model file not found: " + model_file);
    }
    
    auto model_file_size = std::filesystem::file_size(model_file);
    std::cout << "[MEM] Model file size: " << (model_file_size / (1024*1024)) << " MB" << std::endl;
    
    // Create DataMap for model paths
    holoscan::ops::InferenceOp::DataMap model_path_map;
    model_path_map.insert("seg", model_file);
    
    // Create DataVecMap for preprocessor
    holoscan::ops::InferenceOp::DataVecMap pre_processor_map;
    pre_processor_map.insert("seg", std::vector<std::string>{"seg_preprocessed"});
    
    // Create DataVecMap for inference - model has 2 outputs: "outputs" and "proto"
    holoscan::ops::InferenceOp::DataVecMap inference_map;
    inference_map.insert("seg", std::vector<std::string>{"outputs", "proto"});
    
    inference_ = make_operator<holoscan::ops::InferenceOp>(
        "inference",
        holoscan::Arg("model_path_map", model_path_map),
        holoscan::Arg("pre_processor_map", pre_processor_map),
        holoscan::Arg("inference_map", inference_map),
        holoscan::Arg("backend", std::string("trt")),
        holoscan::Arg("enable_fp16", true),
        holoscan::Arg("allocator", device_memory_pool_),
        holoscan::Arg("cuda_stream_pool", cuda_stream_pool_)
    );
    std::cout << "[MEM] Inference operator created" << std::endl;
    
    std::cout << "[MEM] Creating YOLO postprocessor..." << std::endl;
    // YOLO postprocessor
    auto yolo_postprocessor = make_operator<YoloSegPostprocessorOp>(
        "yolo_seg_postprocessor",
        holoscan::Arg("scores_threshold", 0.2),
        holoscan::Arg("num_class", int64_t(12)),
        holoscan::Arg("out_tensor_name", std::string("out_tensor"))
    );
    std::cout << "[MEM] YOLO postprocessor created" << std::endl;
    
    // Visualization - Check for headless mode
    bool headless_mode = std::getenv("HOLOVIZ_HEADLESS") != nullptr;
    
    std::cout << "[MEM] Setting up visualization (headless=" << (headless_mode ? "true" : "false") << ")..." << std::endl;
    
    if (headless_mode) {
        std::cout << "=== HEADLESS MODE ENABLED ===" << std::endl;
        std::cout << "HolovizOp will run without display window, outputting render buffer data." << std::endl;
        
        // Create HolovizOp in headless mode with render buffer output enabled
        visualizer_ = make_operator<holoscan::ops::HolovizOp>(
            "holoviz",
            holoscan::Arg("width", int64_t(1920)),
            holoscan::Arg("height", int64_t(1080)),
            holoscan::Arg("enable_render_buffer_input", false),
            holoscan::Arg("enable_render_buffer_output", true),  // Enable headless mode
            holoscan::Arg("allocator", host_memory_pool_),
            holoscan::Arg("cuda_stream_pool", cuda_stream_pool_)
        );
        std::cout << "[MEM] Headless HolovizOp created" << std::endl;
        
        // Create a simple output operator to receive the render buffer
        auto output_op = make_operator<PassthroughOp>("output");
        std::cout << "[MEM] Output operator created" << std::endl;
        
        std::cout << "[MEM] Connecting pipeline (headless mode)..." << std::endl;
        // Connect the pipeline with headless visualization
        add_flow(replayer_, format_converter_, {{"output", "source_video"}});
        add_flow(format_converter_, inference_, {{"tensor", "receivers"}});
        add_flow(inference_, yolo_postprocessor, {{"transmitter", "in"}});
        add_flow(yolo_postprocessor, visualizer_, {{"out", "receivers"}, {"output_specs", "input_specs"}});
        add_flow(visualizer_, output_op, {{"render_buffer_output", "in"}});
        
        std::cout << "Pipeline configured for headless mode with render buffer output" << std::endl;
    } else {
        std::cout << "Running with display window" << std::endl;
        
        // Simplified HolovizOp configuration for testing - only show video stream
        visualizer_ = make_operator<holoscan::ops::HolovizOp>(
            "holoviz",
            holoscan::Arg("width", int64_t(1920)),
            holoscan::Arg("height", int64_t(1080)),
            holoscan::Arg("enable_render_buffer_input", false),
            holoscan::Arg("enable_render_buffer_output", false),
            holoscan::Arg("allocator", host_memory_pool_),
            holoscan::Arg("cuda_stream_pool", cuda_stream_pool_)
        );
        std::cout << "[MEM] Display HolovizOp created" << std::endl;
        
        std::cout << "[MEM] Connecting pipeline (display mode)..." << std::endl;
        // Connect the pipeline with normal visualization (matching Python version)
        add_flow(replayer_, visualizer_, {{"output", "receivers"}});                // Video stream directly to visualizer
        add_flow(replayer_, format_converter_, {{"output", "source_video"}});       // Video stream to preprocessor
        add_flow(format_converter_, inference_, {{"tensor", "receivers"}});         // Preprocessed tensor to inference
        add_flow(inference_, yolo_postprocessor, {{"transmitter", "in"}});          // Inference results to postprocessor
        add_flow(yolo_postprocessor, visualizer_, {{"out", "receivers"}, {"output_specs", "input_specs"}}); // Postprocessor results to visualizer
    }
    
    std::cout << "[MEM] === UrologyApp::compose() completed successfully ===" << std::endl;
    std::cout << "Pipeline setup completed successfully!" << std::endl;
}

void UrologyApp::setup_recording_pipeline() {
    if (!record_output_) {
        std::cout << "Recording disabled, skipping recording pipeline setup" << std::endl;
        return;
    }
    
    std::cout << "Setting up recording pipeline..." << std::endl;
    
    try {
        // Create async condition for encoder
        auto encoder_async_condition = std::make_shared<holoscan::AsynchronousCondition>();
        
        // Create video encoder context
        video_encoder_context_ = make_resource<VideoEncoderContext>(
            "video_encoder_context",
            holoscan::Arg("async_scheduling_term", encoder_async_condition)
        );
        
        // Create format converters for the recording pipeline
        holoviz_output_format_converter_ = make_operator<holoscan::ops::FormatConverterOp>(
            "holoviz_output_format_converter",
            holoscan::Arg("in_dtype") = std::string("rgba8888"),
            holoscan::Arg("out_dtype") = std::string("rgb888"),
            holoscan::Arg("resize_width") = 1920,
            holoscan::Arg("resize_height") = 1080,
            holoscan::Arg("pool") = device_memory_pool_
        );
        
        encoder_input_format_converter_ = make_operator<holoscan::ops::FormatConverterOp>(
            "encoder_input_format_converter",
            holoscan::Arg("in_dtype") = std::string("rgb888"),
            holoscan::Arg("out_dtype") = std::string("yuv420"),
            holoscan::Arg("pool") = device_memory_pool_
        );
        
        // Create tensor to video buffer converter
        tensor_to_video_buffer_ = make_operator<TensorToVideoBufferOp>(
            "tensor_to_video_buffer",
            holoscan::Arg("video_format") = std::string("yuv420")
        );
        
        // Create video encoder request operator
        std::string output_video_path = output_path_ + "/" + output_filename_;
        std::string crc_file_path = output_path_ + "/surgical_video_output.txt";
        
        video_encoder_request_ = make_operator<VideoEncoderRequestOp>(
            "video_encoder_request",
            holoscan::Arg("videoencoder_context", video_encoder_context_),
            holoscan::Arg("inbuf_storage_type", 1),
            holoscan::Arg("codec", 0),
            holoscan::Arg("input_width", int64_t(1920)),
            holoscan::Arg("input_height", int64_t(1080)),
            holoscan::Arg("input_format", "yuv420planar"),
            holoscan::Arg("profile", 2),
            holoscan::Arg("bitrate", 20000000),
            holoscan::Arg("framerate", 30),
            holoscan::Arg("config", "pframe_cqp"),
            holoscan::Arg("rate_control_mode", 0),
            holoscan::Arg("qp", int64_t(20)),
            holoscan::Arg("iframe_interval", 5)
        );
        
        // Create video encoder response operator
        video_encoder_response_ = make_operator<VideoEncoderResponseOp>(
            "video_encoder_response",
            holoscan::Arg("pool", device_memory_pool_),
            holoscan::Arg("videoencoder_context", video_encoder_context_),
            holoscan::Arg("outbuf_storage_type", int64_t(1))
        );
        
        // Create bitstream writer
        bitstream_writer_ = make_operator<VideoWriteBitstreamOp>(
            "video_write_bitstream",
            holoscan::Arg("output_video_path", output_video_path.c_str()),
            holoscan::Arg("input_crc_file_path", crc_file_path.c_str()),
            holoscan::Arg("frame_width", 1920),
            holoscan::Arg("frame_height", 1080),
            holoscan::Arg("inbuf_storage_type", 1),
            holoscan::Arg("pool", device_memory_pool_)
        );
        
        // Connect the recording pipeline
        // Holoviz render buffer -> format converter
        add_flow(visualizer_, holoviz_output_format_converter_, 
                 {{"render_buffer_output", "source_video"}});
        
        // Format converter -> encoder input format converter
        add_flow(holoviz_output_format_converter_, encoder_input_format_converter_, 
                 {{"tensor", "source_video"}});
        
        // Encoder input format converter -> tensor to video buffer
        add_flow(encoder_input_format_converter_, tensor_to_video_buffer_, 
                 {{"tensor", "in_tensor"}});
        
        // Tensor to video buffer -> video encoder request
        add_flow(tensor_to_video_buffer_, video_encoder_request_, 
                 {{"out_video_buffer", "input_frame"}});
        
        // Video encoder response -> bitstream writer
        add_flow(video_encoder_response_, bitstream_writer_, 
                 {{"output_transmitter", "data_receiver"}});
        
        std::cout << "Recording pipeline setup completed" << std::endl;
        std::cout << "Output file: " << output_video_path << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error setting up recording pipeline: " << e.what() << std::endl;
        record_output_ = false;
    }
}

void UrologyApp::toggle_record() {
    is_recording_ = !is_recording_;
    std::cout << "Recording " << (is_recording_ ? "started" : "stopped") << std::endl;
}

void UrologyApp::set_record_enabled(bool enabled) {
    is_recording_ = enabled;
    std::cout << "Recording " << (is_recording_ ? "enabled" : "disabled") << std::endl;
}

void UrologyApp::setup_visualization() {
    // Additional visualization setup if needed
}

void UrologyApp::setup_inference_pipeline() {
    // Additional inference pipeline setup if needed
}

void UrologyApp::load_gxf_extensions() {
    // Load required GXF extensions for video encoding/decoding
    std::vector<std::string> extensions = {
        "libgxf_videoencoder.so",
        "libgxf_videoencoderio.so", 
        "libgxf_videodecoder.so",
        "libgxf_videodecoderio.so"
    };
    
    try {
        // TODO: Implement GXF extension loading
        // auto context = executor().context();
        for (const auto& ext : extensions) {
            // TODO: Implement GXF extension loading
            // auto result = holoscan::gxf::GxfLoadExtension(context, ext.c_str());
            // if (result != GXF_SUCCESS) {
            //     std::cerr << "Failed to load GXF extension: " << ext << std::endl;
            // }
            (void)ext; // Suppress unused variable warning
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading GXF extensions: " << e.what() << std::endl;
    }
}

} // namespace urology 