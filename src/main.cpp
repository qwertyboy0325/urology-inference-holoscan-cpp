#include "holoscan_fix.hpp"
#include "urology_app.hpp"
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>

namespace {
    std::atomic<bool> should_quit{false};
    
    void keyboard_handler(urology::UrologyApp* app) {
        char key;
        while (!should_quit.load()) {
            std::cin >> key;
            switch (key) {
                case 'p':
                case 'P':
                    app->toggle_pipeline();
                    std::cout << "Pipeline " << (app->is_pipeline_paused() ? "PAUSED" : "RESUMED") << std::endl;
                    break;
                case 'r':
                case 'R':
                    app->toggle_record();
                    std::cout << "Recording " << (app->is_recording() ? "STARTED" : "STOPPED") << std::endl;
                    break;
                case 'q':
                case 'Q':
                    should_quit.store(true);
                    break;
                default:
                    std::cout << "Controls: P=Pause/Resume, R=Record, Q=Quit" << std::endl;
                    break;
            }
        }
    }
}

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
        
        // Start keyboard control in separate thread
        std::thread keyboard_thread(keyboard_handler, app.get());
        
        // Run the application
        app->run();
        
        should_quit.store(true);
        if (keyboard_thread.joinable()) {
            keyboard_thread.join();
    }
    
        std::cout << "Application completed successfully" << std::endl;
    return 0;
        
    } catch (const std::exception& ex) {
        std::cerr << "Application failed: " << ex.what() << std::endl;
        return -1;
    }
} 