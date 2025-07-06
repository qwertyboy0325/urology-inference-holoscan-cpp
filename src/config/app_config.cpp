#include <yaml-cpp/yaml.h>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace urology {
namespace config {

struct AppConfig {
    std::string source = "replayer";
    std::string visualizer = "holoviz";
    std::string record_type = "none";
    bool record_output = true;
    std::string model_type = "yolo_seg_v9";
    std::string model_name = "urology_yolov9c_3000random640resize_20240811_4.34_nhwc.onnx";
    
    struct {
        int width = 1920;
        int height = 1080;
        std::string pixel_format = "nv12";
        bool rdma = true;
        std::string input_type = "hdmi";
    } yuan;
    
    struct {
        std::string basename = "tensor";
        int frame_rate = 30;
        bool repeat = false;
        bool realtime = false;
        int count = 0;
    } replayer;
    
    struct {
        std::string out_tensor_name = "seg_preprocessed";
        std::string out_dtype = "float32";
        std::string in_dtype = "rgb888";
        int resize_width = 640;
        int resize_height = 640;
    } segmentation_preprocessor;
    
    struct {
        std::string backend = "trt";
        bool enable_fp16 = true;
    } inference;
    
    struct {
        float scores_threshold = 0.2f;
        int num_class = 12;
        std::string out_tensor_name = "out_tensor";
    } yolo_postprocessor;
    
    struct {
        int width = 1920;
        int height = 1080;
        std::vector<std::vector<float>> color_lut;
    } viz;
};

class ConfigManager {
public:
    static ConfigManager& instance() {
        static ConfigManager instance;
        return instance;
    }
    
    bool load_config(const std::string& config_path) {
        try {
            if (!std::filesystem::exists(config_path)) {
                std::cerr << "Config file not found: " << config_path << std::endl;
                return false;
            }
            
            YAML::Node config = YAML::LoadFile(config_path);
            
            // Load basic settings
            if (config["source"]) config_.source = config["source"].as<std::string>();
            if (config["visualizer"]) config_.visualizer = config["visualizer"].as<std::string>();
            if (config["record_type"]) config_.record_type = config["record_type"].as<std::string>();
            if (config["record_output"]) config_.record_output = config["record_output"].as<bool>();
            if (config["model_type"]) config_.model_type = config["model_type"].as<std::string>();
            if (config["model_name"]) config_.model_name = config["model_name"].as<std::string>();
            
            // Load yuan settings
            if (config["yuan"]) {
                auto yuan = config["yuan"];
                if (yuan["width"]) config_.yuan.width = yuan["width"].as<int>();
                if (yuan["height"]) config_.yuan.height = yuan["height"].as<int>();
                if (yuan["pixel_format"]) config_.yuan.pixel_format = yuan["pixel_format"].as<std::string>();
                if (yuan["rdma"]) config_.yuan.rdma = yuan["rdma"].as<bool>();
                if (yuan["input_type"]) config_.yuan.input_type = yuan["input_type"].as<std::string>();
            }
            
            // Load replayer settings
            if (config["replayer"]) {
                auto replayer = config["replayer"];
                if (replayer["basename"]) config_.replayer.basename = replayer["basename"].as<std::string>();
                if (replayer["frame_rate"]) config_.replayer.frame_rate = replayer["frame_rate"].as<int>();
                if (replayer["repeat"]) config_.replayer.repeat = replayer["repeat"].as<bool>();
                if (replayer["realtime"]) config_.replayer.realtime = replayer["realtime"].as<bool>();
                if (replayer["count"]) config_.replayer.count = replayer["count"].as<int>();
            }
            
            // Load preprocessing settings
            if (config["segmentation_preprocessor"]) {
                auto preprocessor = config["segmentation_preprocessor"];
                if (preprocessor["out_tensor_name"]) 
                    config_.segmentation_preprocessor.out_tensor_name = preprocessor["out_tensor_name"].as<std::string>();
                if (preprocessor["out_dtype"]) 
                    config_.segmentation_preprocessor.out_dtype = preprocessor["out_dtype"].as<std::string>();
                if (preprocessor["in_dtype"]) 
                    config_.segmentation_preprocessor.in_dtype = preprocessor["in_dtype"].as<std::string>();
                if (preprocessor["resize_width"]) 
                    config_.segmentation_preprocessor.resize_width = preprocessor["resize_width"].as<int>();
                if (preprocessor["resize_height"]) 
                    config_.segmentation_preprocessor.resize_height = preprocessor["resize_height"].as<int>();
            }
            
            // Load inference settings
            if (config["inference"]) {
                auto inference = config["inference"];
                if (inference["backend"]) config_.inference.backend = inference["backend"].as<std::string>();
                if (inference["enable_fp16"]) config_.inference.enable_fp16 = inference["enable_fp16"].as<bool>();
            }
            
            // Load postprocessor settings
            if (config["yolo_postprocessor"]) {
                auto postprocessor = config["yolo_postprocessor"];
                if (postprocessor["scores_threshold"]) 
                    config_.yolo_postprocessor.scores_threshold = postprocessor["scores_threshold"].as<float>();
                if (postprocessor["num_class"]) 
                    config_.yolo_postprocessor.num_class = postprocessor["num_class"].as<int>();
                if (postprocessor["out_tensor_name"]) 
                    config_.yolo_postprocessor.out_tensor_name = postprocessor["out_tensor_name"].as<std::string>();
            }
            
            // Load visualization settings
            if (config["viz"]) {
                auto viz = config["viz"];
                if (viz["width"]) config_.viz.width = viz["width"].as<int>();
                if (viz["height"]) config_.viz.height = viz["height"].as<int>();
                if (viz["color_lut"]) {
                    config_.viz.color_lut.clear();
                    for (const auto& color : viz["color_lut"]) {
                        if (color.IsSequence() && color.size() >= 4) {
                            config_.viz.color_lut.push_back({
                                color[0].as<float>(),
                                color[1].as<float>(),
                                color[2].as<float>(),
                                color[3].as<float>()
                            });
                        }
                    }
                }
            }
            
            loaded_ = true;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Error loading config: " << e.what() << std::endl;
            return false;
        }
    }
    
    const AppConfig& get_config() const {
        return config_;
    }
    
    bool is_loaded() const {
        return loaded_;
    }

private:
    AppConfig config_;
    bool loaded_ = false;
};

} // namespace config
} // namespace urology 