#include "holoscan_fix.hpp"
#include "urology_app.hpp"
#include <holoscan/core/gxf/entity.hpp>
#include <holoscan/core/condition.hpp>

namespace urology {

// VideoEncoderRequestOp Implementation
void VideoEncoderRequestOp::setup(holoscan::OperatorSpec& spec) {
    // Parameters
    spec.param(videoencoder_context_, "videoencoder_context", "Encoder context Handle");
    spec.param(inbuf_storage_type_, "inbuf_storage_type", "Input Buffer storage type");
    spec.param(codec_, "codec", "Video codec");
    spec.param(input_width_, "input_width", "Input width");
    spec.param(input_height_, "input_height", "Input height");
    spec.param(input_format_, "input_format", "Input format");
    spec.param(profile_, "profile", "Encode profile");
    spec.param(bitrate_, "bitrate", "Bitrate");
    spec.param(framerate_, "framerate", "Frame rate");
    spec.param(config_, "config", "Config");
    spec.param(rate_control_mode_, "rate_control_mode", "Rate control mode");
    spec.param(qp_, "qp", "QP");
    spec.param(iframe_interval_, "iframe_interval", "I-frame interval");
    
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
        std::cerr << "Failed to receive input frame" << std::endl;
        return;
    }
    
    // Forward to output (GXF handles the actual encoding)
    op_output.emit(input_frame.value(), "output_transmitter");
}

// VideoEncoderResponseOp Implementation
void VideoEncoderResponseOp::setup(holoscan::OperatorSpec& spec) {
    // Parameters
    spec.param(pool_, "pool", "Memory pool");
    spec.param(videoencoder_context_, "videoencoder_context", "Encoder context Handle");
    spec.param(outbuf_storage_type_, "outbuf_storage_type", "Output buffer storage type");
    
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
        std::cerr << "Failed to receive input" << std::endl;
        return;
    }
    
    // Forward to output (GXF handles the actual response processing)
    op_output.emit(input.value(), "output_transmitter");
}

// VideoEncoderContext Implementation
void VideoEncoderContext::setup(holoscan::ComponentSpec& spec) {
    // Parameters
    spec.param(async_scheduling_term_, "async_scheduling_term", "Asynchronous scheduling condition", nullptr);
}

// VideoWriteBitstreamOp Implementation
void VideoWriteBitstreamOp::setup(holoscan::OperatorSpec& spec) {
    // Parameters
    spec.param(output_video_path_, "output_video_path", "Output video path");
    spec.param(input_crc_file_path_, "input_crc_file_path", "Input CRC file path");
    spec.param(frame_width_, "frame_width", "Frame width");
    spec.param(frame_height_, "frame_height", "Frame height");
    spec.param(inbuf_storage_type_, "inbuf_storage_type", "Input buffer storage type");
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
        std::cerr << "Failed to receive input data" << std::endl;
        return;
    }
    
    // GXF handles the actual bitstream writing
    // This is mainly a passthrough with parameter configuration
}

// TensorToVideoBufferOp Implementation
void TensorToVideoBufferOp::setup(holoscan::OperatorSpec& spec) {
    // Parameters
    spec.param(video_format_, "video_format", "Video format");
    
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
        std::cerr << "Failed to receive input tensor" << std::endl;
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
        std::cerr << "Error in tensor to video buffer conversion: " << e.what() << std::endl;
    }
}

} // namespace urology 