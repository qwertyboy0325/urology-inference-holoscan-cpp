#include "urology_app.hpp"
#include <holoscan/core/gxf/entity.hpp>
#include <holoscan/core/conditions/condition_classes.hpp>

namespace urology {

// VideoEncoderRequestOp Implementation
void VideoEncoderRequestOp::setup(holoscan::OperatorSpec& spec) {
    // Set GXF codelet name
    spec.param(gxf_typename_, std::string("nvidia::gxf::VideoEncoderRequest"));
    
    // Parameters
    spec.param(videoencoder_context_, "videoencoder_context", "Encoder context Handle");
    spec.param(inbuf_storage_type_, "inbuf_storage_type", "Input Buffer storage type", 1);
    spec.param(codec_, "codec", "Video codec", 0);
    spec.param(input_width_, "input_width", "Input width", 1920U);
    spec.param(input_height_, "input_height", "Input height", 1080U);
    spec.param(input_format_, "input_format", "Input format", std::string("yuv420planar"));
    spec.param(profile_, "profile", "Encode profile", 2);
    spec.param(bitrate_, "bitrate", "Bitrate", 20000000);
    spec.param(framerate_, "framerate", "Frame rate", 30);
    spec.param(config_, "config", "Config", std::string("pframe_cqp"));
    spec.param(rate_control_mode_, "rate_control_mode", "Rate control mode", 0);
    spec.param(qp_, "qp", "QP", 20U);
    spec.param(iframe_interval_, "iframe_interval", "I-frame interval", 5);
    
    // Input/Output
    spec.input<holoscan::gxf::Entity>("input_frame");
    spec.output<holoscan::gxf::Entity>("output_transmitter");
}

void VideoEncoderRequestOp::compute(holoscan::InputContext& op_input, 
                                   holoscan::OutputContext& op_output,
                                   holoscan::ExecutionContext&) {
    // Get input frame
    auto input_frame = op_input.receive<holoscan::gxf::Entity>("input_frame");
    if (!input_frame) {
        HOLOSCAN_LOG_ERROR("Failed to receive input frame");
        return;
    }
    
    // Forward to output (GXF handles the actual encoding)
    op_output.emit(input_frame.value(), "output_transmitter");
}

// VideoEncoderResponseOp Implementation
void VideoEncoderResponseOp::setup(holoscan::OperatorSpec& spec) {
    // Set GXF codelet name
    spec.param(gxf_typename_, std::string("nvidia::gxf::VideoEncoderResponse"));
    
    // Parameters
    spec.param(pool_, "pool", "Memory pool");
    spec.param(videoencoder_context_, "videoencoder_context", "Encoder context Handle");
    spec.param(outbuf_storage_type_, "outbuf_storage_type", "Output buffer storage type", 1U);
    
    // Input/Output
    spec.input<holoscan::gxf::Entity>("input_transmitter");
    spec.output<holoscan::gxf::Entity>("output_transmitter");
}

void VideoEncoderResponseOp::compute(holoscan::InputContext& op_input, 
                                    holoscan::OutputContext& op_output,
                                    holoscan::ExecutionContext&) {
    // Get input
    auto input = op_input.receive<holoscan::gxf::Entity>("input_transmitter");
    if (!input) {
        HOLOSCAN_LOG_ERROR("Failed to receive input");
        return;
    }
    
    // Forward to output (GXF handles the actual response processing)
    op_output.emit(input.value(), "output_transmitter");
}

// VideoEncoderContext Implementation
void VideoEncoderContext::setup(holoscan::ComponentSpec& spec) {
    // Set GXF component name
    spec.param(gxf_typename_, std::string("nvidia::gxf::VideoEncoderContext"));
    
    // Parameters
    spec.param(async_scheduling_term_, "async_scheduling_term", 
               "Asynchronous scheduling condition");
}

// VideoWriteBitstreamOp Implementation
void VideoWriteBitstreamOp::setup(holoscan::OperatorSpec& spec) {
    // Set GXF codelet name
    spec.param(gxf_typename_, std::string("nvidia::gxf::VideoWriteBitstream"));
    
    // Parameters
    spec.param(output_video_path_, "output_video_path", "Output video path");
    spec.param(input_crc_file_path_, "input_crc_file_path", "Input CRC file path", 
               std::string(""));
    spec.param(frame_width_, "frame_width", "Frame width", 1920);
    spec.param(frame_height_, "frame_height", "Frame height", 1080);
    spec.param(inbuf_storage_type_, "inbuf_storage_type", "Input buffer storage type", 1);
    spec.param(pool_, "pool", "Memory pool");
    
    // Input
    spec.input<holoscan::gxf::Entity>("data_receiver");
}

void VideoWriteBitstreamOp::compute(holoscan::InputContext& op_input, 
                                   holoscan::OutputContext&,
                                   holoscan::ExecutionContext&) {
    // Get input data
    auto input_data = op_input.receive<holoscan::gxf::Entity>("data_receiver");
    if (!input_data) {
        HOLOSCAN_LOG_ERROR("Failed to receive input data");
        return;
    }
    
    // GXF handles the actual bitstream writing
    // This is mainly a passthrough with parameter configuration
}

// TensorToVideoBufferOp Implementation
void TensorToVideoBufferOp::setup(holoscan::OperatorSpec& spec) {
    // Parameters
    spec.param(video_format_, "video_format", "Video format", std::string("yuv420"));
    
    // Input/Output
    spec.input<holoscan::Tensor>("in_tensor");
    spec.output<holoscan::gxf::Entity>("out_video_buffer");
}

void TensorToVideoBufferOp::compute(holoscan::InputContext& op_input, 
                                   holoscan::OutputContext& op_output,
                                   holoscan::ExecutionContext& context) {
    // Get input tensor
    auto input_tensor = op_input.receive<holoscan::Tensor>("in_tensor");
    if (!input_tensor) {
        HOLOSCAN_LOG_ERROR("Failed to receive input tensor");
        return;
    }
    
    // Convert tensor to video buffer
    // This is a simplified implementation - in a real scenario, you'd need to
    // properly convert the tensor data to the specified video format
    try {
        // Create a GXF entity to hold the video buffer
        auto entity = holoscan::gxf::Entity::New(&context);
        
        // For now, we'll create a basic passthrough
        // In a full implementation, you would:
        // 1. Get tensor data and dimensions
        // 2. Convert to specified video format (YUV420, etc.)
        // 3. Create video buffer component in the entity
        // 4. Copy converted data to the buffer
        
        // Emit the entity
        op_output.emit(entity, "out_video_buffer");
        
    } catch (const std::exception& e) {
        HOLOSCAN_LOG_ERROR("Error in tensor to video buffer conversion: {}", e.what());
    }
}

} // namespace urology 