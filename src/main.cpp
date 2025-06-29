#include <holoscan/holoscan.hpp>
#include <iostream>
#include <memory>

#include "urology_app.hpp"

int main(int argc, char** argv) {
    // Parse command line arguments
    std::string data_path = "none";
    std::string source = "replayer";
    std::string output_filename = "";
    std::string labels_file = "";
    
    // Simple argument parsing
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-d" || arg == "--data") {
            if (i + 1 < argc) {
                data_path = argv[++i];
            }
        } else if (arg == "-s" || arg == "--source") {
            if (i + 1 < argc) {
                source = argv[++i];
            }
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                output_filename = argv[++i];
            }
        } else if (arg == "-l" || arg == "--labels") {
            if (i + 1 < argc) {
                labels_file = argv[++i];
            }
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]\n"
                      << "Options:\n"
                      << "  -d, --data PATH      Set the input data path\n"
                      << "  -s, --source TYPE    Set source type (replayer/yuan)\n"
                      << "  -o, --output FILE    Set output filename\n"
                      << "  -l, --labels FILE    Set labels file path\n"
                      << "  -h, --help          Show this help message\n";
            return 0;
        }
    }
    
    try {
        auto app = std::make_shared<urology::UrologyApp>(
            data_path, source, output_filename, labels_file);
        
        // Initialize Holoscan
        holoscan::set_log_level(holoscan::LogLevel::INFO);
        
        std::cout << "Starting Urology Inference Holoscan Application..." << std::endl;
        std::cout << "Data path: " << data_path << std::endl;
        std::cout << "Source: " << source << std::endl;
        
        app->run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 