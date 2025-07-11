#include "holoscan_fix.hpp"
#include "urology_app.hpp"
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <sys/resource.h>

namespace {
    std::atomic<bool> should_quit{false};
    
    // Function to get current memory usage
    void log_memory_usage(const std::string& phase) {
        struct rusage r_usage;
        if (getrusage(RUSAGE_SELF, &r_usage) == 0) {
            long memory_kb = r_usage.ru_maxrss;
            std::cout << "[MEM] " << phase << " - Memory usage: " << (memory_kb / 1024) << " MB" << std::endl;
        }
    }
    
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
    std::cout << "[MEM] === Starting Urology Inference Application ===" << std::endl;
    std::cout << "[MEM] Application entry point reached" << std::endl;
    log_memory_usage("Application start");
    
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
    
    std::cout << "[MEM] Command line arguments parsed" << std::endl;
    std::cout << "Data path: " << data_path << std::endl;
    std::cout << "Source: " << source << std::endl;
    log_memory_usage("After argument parsing");
    
    try {
        std::cout << "[MEM] Creating UrologyApp instance..." << std::endl;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        auto app = std::make_shared<urology::UrologyApp>(
            data_path, source, output_filename, labels_file);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "[MEM] UrologyApp instance created in " << duration.count() << " ms" << std::endl;
        log_memory_usage("After UrologyApp creation");
        
        // Initialize Holoscan
        std::cout << "[MEM] Initializing Holoscan..." << std::endl;
        start_time = std::chrono::high_resolution_clock::now();
        
        holoscan::set_log_level(holoscan::LogLevel::INFO);
        
        end_time = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "[MEM] Holoscan initialized in " << duration.count() << " ms" << std::endl;
        log_memory_usage("After Holoscan initialization");
        
        std::cout << "Starting Urology Inference Holoscan Application..." << std::endl;
        
        // TEMPORARILY DISABLE KEYBOARD HANDLER TO PREVENT INFINITE LOOP
        // std::cout << "[MEM] Starting keyboard control thread..." << std::endl;
        // std::thread keyboard_thread(keyboard_handler, app.get());
        
        // Run the application
        std::cout << "[MEM] Starting application run..." << std::endl;
        start_time = std::chrono::high_resolution_clock::now();
        log_memory_usage("Before app->run()");
        
        app->run();
        
        end_time = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "[MEM] Application run completed in " << duration.count() << " ms" << std::endl;
        log_memory_usage("After app->run()");
        
        should_quit.store(true);
        // if (keyboard_thread.joinable()) {
        //     keyboard_thread.join();
        // }
    
        std::cout << "[MEM] === Application completed successfully ===" << std::endl;
        log_memory_usage("Application end");
        std::cout << "Application completed successfully" << std::endl;
        return 0;
        
    } catch (const std::exception& ex) {
        std::cerr << "[MEM] === Application failed with exception ===" << std::endl;
        log_memory_usage("After exception");
        std::cerr << "Application failed: " << ex.what() << std::endl;
        return -1;
    }
} 