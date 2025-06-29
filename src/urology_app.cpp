#include "urology_app.hpp"
#include "operators/yolo_seg_postprocessor.hpp"

#include <yaml-cpp/yaml.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <holoscan/core/gxf/gxf_utils.hpp>

namespace urology {

UrologyApp::UrologyApp(const std::string& data_path, 
                      const std::string& source,
                      const std::string& output_filename,
                      const std::string& labels_file)
    : data_path_(data_path), 
      output_filename_(output_filename),
      labels_file_(labels_file),
      is_recording_(true),
      source_type_(source),
      visualizer_type_("holoviz"),
      record_output_(true),
      model_type_("yolo_seg_v9"),
      model_name_("Urology_yolov11x-seg_3-13-16-17val640rezize_1_4.40.0_nms.onnx") {
    
    // Set name
    name("Urology App");
    
    // Set up paths
    if (data_path_ == "none") {
        const char* env_path = std::getenv("HOLOHUB_DATA_PATH");
        data_path_ = env_path ? env_path : "../data";
    }
    
    const char* model_env = std::getenv("HOLOSCAN_MODEL_PATH");
    std::string model_base = model_env ? model_env : "../data/models";
    
    model_path_ = std::filesystem::path(data_path_) / "models";
    output_path_ = std::filesystem::path(data_path_) / "output";
    
    // Create output directory if it doesn't exist
    std::filesystem::create_directories(output_path_);
    
    // Load labels
    load_labels();
    
    // Initialize memory pools
    host_memory_pool_ = std::make_shared<holoscan::BlockMemoryPool>(
        "host_memory_pool", 1, 32 * 1024 * 1024, 2);
    device_memory_pool_ = std::make_shared<holoscan::BlockMemoryPool>(
        "device_memory_pool", 1, 32 * 1024 * 1024, 2);
    cuda_stream_pool_ = std::make_shared<holoscan::CudaStreamPool>("cuda_stream", 0, 0, 0, 1, 5);
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
    // Load required GXF extensions first
    load_gxf_extensions();
    
    // Get configuration parameters
    source_type_ = from_config("source").value_or("replayer");
    record_output_ = from_config("record_output").value_or(true);
    
    if (source_type_ == "yuan") {
        // TODO: Implement YUAN capture card support
        std::cerr << "YUAN capture card support not implemented yet" << std::endl;
        throw std::runtime_error("YUAN capture card not supported");
    } else {
        // Use video stream replayer
        std::string video_dir = std::filesystem::path(data_path_) / "inputs";
        if (!std::filesystem::exists(video_dir)) {
            throw std::runtime_error("Could not find video data: " + video_dir);
        }
        
        replayer_ = make_operator<VideoStreamReplayerOp>(
            "replayer",
            Arg("directory") = video_dir,
            Arg("basename") = std::string("tensor"),
            Arg("frame_rate") = 30.0f,
            Arg("repeat") = false,
            Arg("realtime") = false,
            Arg("count") = 0
        );
    }
    
    // Format converter for preprocessing
    format_converter_ = make_operator<FormatConverterOp>(
        "segmentation_preprocessor",
        Arg("out_tensor_name") = std::string("seg_preprocessed"),
        Arg("out_dtype") = std::string("float32"),
        Arg("in_dtype") = std::string("rgb888"),
        Arg("resize_width") = 640,
        Arg("resize_height") = 640,
        Arg("pool") = device_memory_pool_
    );
    
    // Inference operator
    std::string model_file = std::filesystem::path(model_path_) / model_name_;
    inference_ = make_operator<InferenceOp>(
        "inference",
        Arg("model_path_map") = std::map<std::string, std::string>{{"seg", model_file}},
        Arg("pre_processor_map") = std::map<std::string, std::vector<std::string>>{
            {"seg", {"seg_preprocessed"}}
        },
        Arg("backend") = std::string("trt"),
        Arg("enable_fp16") = true,
        Arg("allocator") = device_memory_pool_,
        Arg("cuda_stream_pool") = cuda_stream_pool_
    );
    
    // YOLO postprocessor
    auto yolo_postprocessor = make_operator<YoloSegPostprocessorOp>(
        "yolo_postprocessor",
        label_dict_,
        Arg("scores_threshold") = 0.2f,
        Arg("num_class") = 12,
        Arg("out_tensor_name") = std::string("out_tensor"),
        Arg("allocator") = device_memory_pool_
    );
    
    // Visualization
    visualizer_ = make_operator<HolovizOp>(
        "holoviz",
        Arg("width") = 1920,
        Arg("height") = 1080,
        Arg("enable_render_buffer_input") = false,
        Arg("enable_render_buffer_output") = record_output_,
        Arg("allocator") = record_output_ ? device_memory_pool_ : host_memory_pool_,
        Arg("cuda_stream_pool") = cuda_stream_pool_
    );
    
    // Connect the pipeline
    if (replayer_) {
        add_flow(replayer_, format_converter_, {{"output", "source_video"}});
    }
    
    add_flow(format_converter_, inference_, {{"tensor", "receivers"}});
    add_flow(inference_, yolo_postprocessor, {{"transmitter", "in"}});
    add_flow(yolo_postprocessor, visualizer_, {{"out", "receivers"}});
    
    // Add recording pipeline if enabled
    if (record_output_) {
        setup_recording_pipeline();
    }
}

void UrologyApp::setup_recording_pipeline() {
    if (!record_output_) {
        std::cout << "Recording disabled, skipping recording pipeline setup" << std::endl;
        return;
    }
    
    std::cout << "Setting up recording pipeline..." << std::endl;
    
    try {
        // Create async condition for encoder
        auto encoder_async_condition = std::make_shared<holoscan::AsynchronousCondition>(
            "encoder_async_condition");
        
        // Create video encoder context
        video_encoder_context_ = std::make_shared<VideoEncoderContext>(
            "video_encoder_context",
            Arg("async_scheduling_term") = encoder_async_condition
        );
        
        // Create format converters for the recording pipeline
        holoviz_output_format_converter_ = make_operator<holoscan::ops::FormatConverterOp>(
            "holoviz_output_format_converter",
            Arg("in_dtype") = std::string("rgba8888"),
            Arg("out_dtype") = std::string("rgb888"),
            Arg("resize_width") = 1920,
            Arg("resize_height") = 1080,
            Arg("pool") = device_memory_pool_
        );
        
        encoder_input_format_converter_ = make_operator<holoscan::ops::FormatConverterOp>(
            "encoder_input_format_converter",
            Arg("in_dtype") = std::string("rgb888"),
            Arg("out_dtype") = std::string("yuv420"),
            Arg("pool") = device_memory_pool_
        );
        
        // Create tensor to video buffer converter
        tensor_to_video_buffer_ = make_operator<TensorToVideoBufferOp>(
            "tensor_to_video_buffer",
            Arg("video_format") = std::string("yuv420")
        );
        
        // Create video encoder request operator
        video_encoder_request_ = make_operator<VideoEncoderRequestOp>(
            "video_encoder_request",
            Arg("videoencoder_context") = video_encoder_context_,
            Arg("inbuf_storage_type") = 1,
            Arg("codec") = 0,
            Arg("input_width") = 1920U,
            Arg("input_height") = 1080U,
            Arg("input_format") = std::string("yuv420planar"),
            Arg("profile") = 2,
            Arg("bitrate") = 20000000,
            Arg("framerate") = 30,
            Arg("config") = std::string("pframe_cqp"),
            Arg("rate_control_mode") = 0,
            Arg("qp") = 20U,
            Arg("iframe_interval") = 5
        );
        
        // Create video encoder response operator
        video_encoder_response_ = make_operator<VideoEncoderResponseOp>(
            "video_encoder_response",
            Arg("pool") = device_memory_pool_,
            Arg("videoencoder_context") = video_encoder_context_,
            Arg("outbuf_storage_type") = 1U
        );
        
        // Create bitstream writer
        std::string output_video_path = output_path_ + "/" + output_filename_;
        std::string crc_file_path = output_path_ + "/surgical_video_output.txt";
        
        bitstream_writer_ = make_operator<VideoWriteBitstreamOp>(
            "bitstream_writer",
            Arg("output_video_path") = output_video_path,
            Arg("input_crc_file_path") = crc_file_path,
            Arg("frame_width") = 1920,
            Arg("frame_height") = 1080,
            Arg("inbuf_storage_type") = 1,
            Arg("pool") = device_memory_pool_
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

void UrologyApp::create_passthrough_ops(int count) {
    // TODO: Implement passthrough operators for pipeline control
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
        auto context = executor().context();
        for (const auto& ext : extensions) {
            auto result = holoscan::gxf::GxfLoadExtension(context, ext.c_str());
            if (result != GXF_SUCCESS) {
                HOLOSCAN_LOG_WARN("Failed to load GXF extension: {}", ext);
            } else {
                HOLOSCAN_LOG_INFO("Loaded GXF extension: {}", ext);
            }
        }
    } catch (const std::exception& e) {
        HOLOSCAN_LOG_ERROR("Error loading GXF extensions: {}", e.what());
    }
}

} // namespace urology 