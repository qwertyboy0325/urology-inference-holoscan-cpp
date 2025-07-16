#pragma once

#include <holoscan/holoscan.hpp>
#include <holoscan/operators/video_stream_replayer/video_stream_replayer.hpp>
#include <holoscan/operators/format_converter/format_converter.hpp>
#include <holoscan/operators/inference/inference.hpp>
#include <holoscan/operators/holoviz/holoviz.hpp>
#include <holoscan/operators/segmentation_postprocessor/segmentation_postprocessor.hpp>
#include <holoscan/operators/gxf_codelet/gxf_codelet.hpp>
#include <holoscan/core/gxf/gxf_component.hpp>
#include <holoscan/core/condition.hpp>

// Include our new HolovizOp-native postprocessor
#include "operators/holoviz_native_yolo_postprocessor.hpp"
#include "operators/holoviz_static_config.hpp"
#include "operators/holoviz_common_types.hpp"

#include <string>
#include <map>
#include <vector>
#include <memory>

namespace urology {

// Forward declarations for video encoder components
class VideoEncoderRequestOp;
class VideoEncoderResponseOp;
class VideoEncoderContext;
class VideoWriteBitstreamOp;
class TensorToVideoBufferOp;

// Forward declaration for legacy postprocessor
class YoloSegPostprocessorOp;

class UrologyApp : public holoscan::Application {
public:
    explicit UrologyApp(const std::string& data_path = "none", 
                       const std::string& source = "replayer",
                       const std::string& output_filename = "",
                       const std::string& labels_file = "");

    void compose() override;

    // Pipeline control (simplified)
    void pause_pipeline() { 
        std::cout << "Pipeline pause requested" << std::endl;
    }
    
    void resume_pipeline() { 
        std::cout << "Pipeline resume requested" << std::endl;
    }
    
    void toggle_pipeline() { 
        std::cout << "Pipeline toggle requested" << std::endl;
    }
    
    bool is_pipeline_paused() const { 
        return false; // Always running for now
    }

    // Recording control (separate from pipeline pause)
    void toggle_record();
    void set_record_enabled(bool enabled);
    bool is_recording() const { return is_recording_; }

private:
    void load_labels();
    void setup_visualization();
    void setup_inference_pipeline();
    void setup_recording_pipeline();
    void load_gxf_extensions();
    void setup_holoviz_static_config();

    // Configuration
    std::string data_path_;
    std::string model_path_;
    std::string input_path_;
    std::string output_path_;
    std::string output_filename_;
    std::string labels_file_;
    
    // Runtime state
    bool is_recording_;
    std::string source_type_;
    std::string visualizer_type_;
    std::string model_name_;
    std::string model_type_;
    bool record_output_;
    
    // Pipeline control state
    bool pipeline_paused_;
    
    // Label dictionary
    LabelDict label_dict_;
    
    // Static tensor configuration for HolovizOp
    std::vector<holoscan::ops::HolovizOp::InputSpec> static_tensor_config_;
    
    // Operators
    std::shared_ptr<holoscan::ops::VideoStreamReplayerOp> replayer_;
    std::shared_ptr<holoscan::ops::FormatConverterOp> format_converter_;
    std::shared_ptr<holoscan::ops::InferenceOp> inference_;
    std::shared_ptr<holoscan::ops::HolovizOp> visualizer_;
    
    // New HolovizOp-native postprocessor (replaces old YoloSegPostprocessorOp)
    std::shared_ptr<HolovizNativeYoloPostprocessor> holoviz_native_postprocessor_;
    
    // Legacy postprocessor (for comparison/testing)
    std::shared_ptr<YoloSegPostprocessorOp> legacy_postprocessor_;
    
    // Video encoder operators (for recording)
    std::shared_ptr<VideoEncoderRequestOp> video_encoder_request_;
    std::shared_ptr<VideoEncoderResponseOp> video_encoder_response_;
    std::shared_ptr<VideoEncoderContext> video_encoder_context_;
    std::shared_ptr<VideoWriteBitstreamOp> bitstream_writer_;
    std::shared_ptr<TensorToVideoBufferOp> tensor_to_video_buffer_;
    std::shared_ptr<holoscan::ops::FormatConverterOp> holoviz_output_format_converter_;
    std::shared_ptr<holoscan::ops::FormatConverterOp> encoder_input_format_converter_;
    
    // Memory pools
    std::shared_ptr<holoscan::BlockMemoryPool> host_memory_pool_;
    std::shared_ptr<holoscan::BlockMemoryPool> device_memory_pool_;
    std::shared_ptr<holoscan::CudaStreamPool> cuda_stream_pool_;
};

// Video encoder operator declarations (GXF-based)
class VideoEncoderRequestOp : public holoscan::ops::GXFCodeletOp {
public:
    template<typename ArgT, typename... ArgsT>
    explicit VideoEncoderRequestOp(ArgT&& arg, ArgsT&&... args)
        : holoscan::ops::GXFCodeletOp("nvidia::gxf::VideoEncoderRequest", 
                                     std::forward<ArgT>(arg), 
                                     std::forward<ArgsT>(args)...) {}
    
    VideoEncoderRequestOp() : holoscan::ops::GXFCodeletOp("nvidia::gxf::VideoEncoderRequest") {}
    
    void setup(holoscan::OperatorSpec& spec) override;
    void compute(holoscan::InputContext&, holoscan::OutputContext&,
                 holoscan::ExecutionContext&) override;
    
private:
    holoscan::Parameter<std::shared_ptr<holoscan::Resource>> videoencoder_context_;
    holoscan::Parameter<int64_t> inbuf_storage_type_;
    holoscan::Parameter<int64_t> codec_;
    holoscan::Parameter<int64_t> input_width_;
    holoscan::Parameter<int64_t> input_height_;
    holoscan::Parameter<std::string> input_format_;
    holoscan::Parameter<int64_t> profile_;
    holoscan::Parameter<int64_t> bitrate_;
    holoscan::Parameter<int64_t> framerate_;
    holoscan::Parameter<std::string> config_;
    holoscan::Parameter<int64_t> rate_control_mode_;
    holoscan::Parameter<int64_t> qp_;
    holoscan::Parameter<int64_t> iframe_interval_;
};

class VideoEncoderResponseOp : public holoscan::ops::GXFCodeletOp {
public:
    template<typename ArgT, typename... ArgsT>
    explicit VideoEncoderResponseOp(ArgT&& arg, ArgsT&&... args)
        : holoscan::ops::GXFCodeletOp("nvidia::gxf::VideoEncoderResponse", 
                                     std::forward<ArgT>(arg), 
                                     std::forward<ArgsT>(args)...) {}
    
    VideoEncoderResponseOp() : holoscan::ops::GXFCodeletOp("nvidia::gxf::VideoEncoderResponse") {}
    
    void setup(holoscan::OperatorSpec& spec) override;
    void compute(holoscan::InputContext&, holoscan::OutputContext&,
                 holoscan::ExecutionContext&) override;
    
private:
    holoscan::Parameter<std::shared_ptr<holoscan::Allocator>> pool_;
    holoscan::Parameter<std::shared_ptr<holoscan::Resource>> videoencoder_context_;
    holoscan::Parameter<int64_t> outbuf_storage_type_;
};

class VideoEncoderContext : public holoscan::gxf::GXFResource {
public:
    HOLOSCAN_RESOURCE_FORWARD_ARGS_SUPER(VideoEncoderContext, holoscan::gxf::GXFResource)
    VideoEncoderContext() = default;
    void setup(holoscan::ComponentSpec& spec) override;
private:
    holoscan::Parameter<std::shared_ptr<holoscan::AsynchronousCondition>> async_scheduling_term_;
};

class VideoWriteBitstreamOp : public holoscan::ops::GXFCodeletOp {
public:
    template<typename ArgT, typename... ArgsT>
    explicit VideoWriteBitstreamOp(ArgT&& arg, ArgsT&&... args)
        : holoscan::ops::GXFCodeletOp("nvidia::gxf::VideoWriteBitstream", 
                                     std::forward<ArgT>(arg), 
                                     std::forward<ArgsT>(args)...) {}
    
    VideoWriteBitstreamOp() : holoscan::ops::GXFCodeletOp("nvidia::gxf::VideoWriteBitstream") {}
    
    void setup(holoscan::OperatorSpec& spec) override;
    void compute(holoscan::InputContext&, holoscan::OutputContext&,
                 holoscan::ExecutionContext&) override;
    
private:
    holoscan::Parameter<std::string> output_video_path_;
    holoscan::Parameter<std::string> input_crc_file_path_;
    holoscan::Parameter<int64_t> frame_width_;
    holoscan::Parameter<int64_t> frame_height_;
    holoscan::Parameter<int64_t> inbuf_storage_type_;
    holoscan::Parameter<std::shared_ptr<holoscan::Allocator>> pool_;
};

class TensorToVideoBufferOp : public holoscan::Operator {
public:
    HOLOSCAN_OPERATOR_FORWARD_ARGS(TensorToVideoBufferOp)
    
    TensorToVideoBufferOp() = default;
    
    void setup(holoscan::OperatorSpec& spec) override;
    void compute(holoscan::InputContext&, holoscan::OutputContext&,
                 holoscan::ExecutionContext&) override;
    
private:
    holoscan::Parameter<std::string> video_format_;
};

// Simple passthrough operator for headless mode
class PassthroughOp : public holoscan::Operator {
public:
    HOLOSCAN_OPERATOR_FORWARD_ARGS(PassthroughOp)

    PassthroughOp() = default;

    void setup(holoscan::OperatorSpec& spec) override {
        spec.input<holoscan::TensorMap>("in");
        spec.output<holoscan::TensorMap>("out");
    }

    void compute(holoscan::InputContext& op_input, 
                holoscan::OutputContext& op_output,
                holoscan::ExecutionContext&) override {
        auto in_message = op_input.receive<holoscan::TensorMap>("in");
        if (in_message) {
            std::cout << "✓ Frame processed successfully in headless mode!" << std::endl;
            op_output.emit(in_message.value(), "out");
        }
    }
};

} // namespace urology 