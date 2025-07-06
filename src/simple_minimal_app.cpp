#include "holoscan_fix.hpp"
#include <holoscan/operators/video_stream_replayer/video_stream_replayer.hpp>
#include <holoscan/operators/format_converter/format_converter.hpp>
#include <iostream>

class MinimalUrologyApp : public holoscan::Application {
public:
    MinimalUrologyApp() = default;

    void compose() override {
        std::cout << "=== Minimal Urology Application ===" << std::endl;
        
        // Create a simple replayer
        auto replayer = make_operator<holoscan::ops::VideoStreamReplayerOp>(
            "replayer",
            holoscan::Arg("directory", std::string("/workspace/data/inputs")),
            holoscan::Arg("basename", std::string("tensor")),
            holoscan::Arg("frame_rate", 30.0f),
            holoscan::Arg("repeat", false),
            holoscan::Arg("realtime", false),
            holoscan::Arg("count", 5UL)  // Only process 5 frames for testing
        );
        
        // Create a format converter
        auto converter = make_operator<holoscan::ops::FormatConverterOp>(
            "converter",
            holoscan::Arg("out_dtype", std::string("float32")),
            holoscan::Arg("out_tensor_name", std::string("source_video"))
        );
        
        // Add operators to the application
        add_operator(replayer);
        add_operator(converter);
        
        // Connect the operators
        add_flow(replayer, converter);
        
        std::cout << "Application composition completed." << std::endl;
    }
};

int main() {
    try {
        std::cout << "Starting Minimal Urology Application..." << std::endl;
        
        MinimalUrologyApp app;
        app.run();
        
        std::cout << "Application completed successfully." << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Application failed: " << ex.what() << std::endl;
        return -1;
    }
} 