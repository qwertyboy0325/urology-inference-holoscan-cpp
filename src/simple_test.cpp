#include <iostream>
#include <string>
#include <holoscan/holoscan.hpp>
#include <holoscan/operators/video_stream_replayer/video_stream_replayer.hpp>
#include <holoscan/operators/format_converter/format_converter.hpp>
#include <holoscan/operators/holoviz/holoviz.hpp>

class SimpleUrologyApp : public holoscan::Application {
public:
    SimpleUrologyApp(const std::string& data_path) : data_path_(data_path) {}

    void compose() override {
        std::cout << "=== Simple Urology Test Application ===" << std::endl;
        std::cout << "Data path: " << data_path_ << std::endl;
        
        // Create a simple replayer
        auto replayer = make_operator<holoscan::ops::VideoStreamReplayerOp>(
            "replayer",
            holoscan::Arg("directory", data_path_ + "/inputs"),
            holoscan::Arg("basename", std::string("tensor")),
            holoscan::Arg("frame_rate", 30.0f),
            holoscan::Arg("repeat", false),
            holoscan::Arg("realtime", false),
            holoscan::Arg("count", 10)  // Only process 10 frames
        );
        
        // Create a format converter
        auto format_converter = make_operator<holoscan::ops::FormatConverterOp>(
            "format_converter",
            holoscan::Arg("out_tensor_name", std::string("converted")),
            holoscan::Arg("out_dtype", std::string("float32")),
            holoscan::Arg("in_dtype", std::string("rgb888"))
        );
        
        // Create a simple visualizer
        auto visualizer = make_operator<holoscan::ops::HolovizOp>(
            "visualizer",
            holoscan::Arg("width", 640),
            holoscan::Arg("height", 640),
            holoscan::Arg("window_title", std::string("Simple Urology Test"))
        );
        
        // Connect operators
        add_flow(replayer, format_converter, {{"output", "source_video"}});
        add_flow(format_converter, visualizer, {{"tensor", "receivers"}});
        
        std::cout << "Pipeline configured successfully!" << std::endl;
    }

private:
    std::string data_path_;
};

int main(int argc, char** argv) {
    std::string data_path = "./data";
    
    if (argc > 1) {
        data_path = argv[1];
    }
    
    std::cout << "Starting simple urology test application..." << std::endl;
    std::cout << "Data path: " << data_path << std::endl;
    
    try {
        auto app = holoscan::make_application<SimpleUrologyApp>(data_path);
        app->run();
        std::cout << "Application completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 