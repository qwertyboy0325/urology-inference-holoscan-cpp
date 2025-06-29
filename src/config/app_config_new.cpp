#include "config/app_config.hpp"
#include <iostream>
#include <filesystem>
#include <cstdlib>

namespace urology {
namespace config {

bool AppConfig::loadFromFile(const std::string& config_file) {
    try {
        if (!std::filesystem::exists(config_file)) {
            std::cerr << "Configuration file not found: " << config_file << std::endl;
            setDefaults();
            return false;
        }
        
        config_ = YAML::LoadFile(config_file);
        
        // Parse different sections
        parseVideoConfig(config_["yuan"]);
        parseInferenceConfig(config_);
        parseEncoderConfig(config_);
        parseMemoryConfig(config_["memory"]);
        
        return true;
        
    } catch (const YAML::Exception& e) {
        std::cerr << "YAML parsing error: " << e.what() << std::endl;
        setDefaults();
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error loading configuration: " << e.what() << std::endl;
        setDefaults();
        return false;
    }
}

void AppConfig::loadFromEnvironment() {
    // Load paths from environment variables
    if (const char* data_path = std::getenv("HOLOHUB_DATA_PATH")) {
        data_path_ = data_path;
    }
    
    if (const char* model_path = std::getenv("HOLOSCAN_MODEL_PATH")) {
        model_path_ = model_path;
    }
    
    // Load other configuration from environment
    if (const char* record_output = std::getenv("UROLOGY_RECORD_OUTPUT")) {
        encoder_config_.record_output = (std::string(record_output) == "true");
    }
    
    if (const char* model_name = std::getenv("UROLOGY_MODEL_NAME")) {
        inference_config_.model_name = model_name;
    }
    
    if (const char* backend = std::getenv("UROLOGY_INFERENCE_BACKEND")) {
        inference_config_.backend = backend;
    }
}

bool AppConfig::validate() const {
    // Validate video configuration
    if (video_config_.width <= 0 || video_config_.height <= 0) {
        std::cerr << "Invalid video dimensions: " << video_config_.width 
                  << "x" << video_config_.height << std::endl;
        return false;
    }
    
    // Validate inference configuration
    if (inference_config_.scores_threshold < 0.0f || inference_config_.scores_threshold > 1.0f) {
        std::cerr << "Invalid scores threshold: " << inference_config_.scores_threshold << std::endl;
        return false;
    }
    
    if (inference_config_.num_classes <= 0) {
        std::cerr << "Invalid number of classes: " << inference_config_.num_classes << std::endl;
        return false;
    }
    
    // Validate encoder configuration
    if (encoder_config_.bitrate <= 0) {
        std::cerr << "Invalid bitrate: " << encoder_config_.bitrate << std::endl;
        return false;
    }
    
    if (encoder_config_.framerate <= 0) {
        std::cerr << "Invalid framerate: " << encoder_config_.framerate << std::endl;
        return false;
    }
    
    // Validate memory configuration
    if (memory_config_.host_memory_pool_size == 0 || memory_config_.device_memory_pool_size == 0) {
        std::cerr << "Invalid memory pool sizes" << std::endl;
        return false;
    }
    
    return true;
}

std::optional<std::string> AppConfig::getString(const std::string& key) const {
    if (config_[key]) {
        return config_[key].as<std::string>();
    }
    return std::nullopt;
}

std::optional<int> AppConfig::getInt(const std::string& key) const {
    if (config_[key]) {
        return config_[key].as<int>();
    }
    return std::nullopt;
}

std::optional<bool> AppConfig::getBool(const std::string& key) const {
    if (config_[key]) {
        return config_[key].as<bool>();
    }
    return std::nullopt;
}

void AppConfig::setDefaults() {
    // Set default paths
    data_path_ = "../data";
    model_path_ = "../data/models";
    output_path_ = "../data/output";
    labels_file_ = "../data/config/labels.yaml";
    
    // Other defaults are already set in struct definitions
}

void AppConfig::parseVideoConfig(const YAML::Node& node) {
    if (!node) return;
    
    if (node["width"]) video_config_.width = node["width"].as<int>();
    if (node["height"]) video_config_.height = node["height"].as<int>();
    if (node["pixel_format"]) video_config_.pixel_format = node["pixel_format"].as<std::string>();
    if (node["rdma"]) video_config_.rdma = node["rdma"].as<bool>();
    if (node["input_type"]) video_config_.input_type = node["input_type"].as<std::string>();
}

void AppConfig::parseInferenceConfig(const YAML::Node& node) {
    if (!node) return;
    
    if (node["model_type"]) inference_config_.model_type = node["model_type"].as<std::string>();
    if (node["model_name"]) inference_config_.model_name = node["model_name"].as<std::string>();
    
    if (node["inference"]) {
        auto inference_node = node["inference"];
        if (inference_node["backend"]) inference_config_.backend = inference_node["backend"].as<std::string>();
        if (inference_node["enable_fp16"]) inference_config_.enable_fp16 = inference_node["enable_fp16"].as<bool>();
    }
    
    if (node["yolo_postprocessor"]) {
        auto yolo_node = node["yolo_postprocessor"];
        if (yolo_node["scores_threshold"]) inference_config_.scores_threshold = yolo_node["scores_threshold"].as<float>();
        if (yolo_node["num_class"]) inference_config_.num_classes = yolo_node["num_class"].as<int>();
    }
}

void AppConfig::parseEncoderConfig(const YAML::Node& node) {
    if (!node) return;
    
    if (node["record_output"]) encoder_config_.record_output = node["record_output"].as<bool>();
    
    if (node["video_encoder_request"]) {
        auto encoder_node = node["video_encoder_request"];
        if (encoder_node["inbuf_storage_type"]) encoder_config_.inbuf_storage_type = encoder_node["inbuf_storage_type"].as<int>();
        if (encoder_node["codec"]) encoder_config_.codec = encoder_node["codec"].as<int>();
        if (encoder_node["input_width"]) encoder_config_.input_width = encoder_node["input_width"].as<int>();
        if (encoder_node["input_height"]) encoder_config_.input_height = encoder_node["input_height"].as<int>();
        if (encoder_node["input_format"]) encoder_config_.input_format = encoder_node["input_format"].as<std::string>();
        if (encoder_node["profile"]) encoder_config_.profile = encoder_node["profile"].as<int>();
        if (encoder_node["bitrate"]) encoder_config_.bitrate = encoder_node["bitrate"].as<int>();
        if (encoder_node["framerate"]) encoder_config_.framerate = encoder_node["framerate"].as<int>();
        if (encoder_node["config"]) encoder_config_.config = encoder_node["config"].as<std::string>();
        if (encoder_node["rate_control_mode"]) encoder_config_.rate_control_mode = encoder_node["rate_control_mode"].as<int>();
        if (encoder_node["qp"]) encoder_config_.qp = encoder_node["qp"].as<int>();
        if (encoder_node["iframe_interval"]) encoder_config_.iframe_interval = encoder_node["iframe_interval"].as<int>();
    }
}

void AppConfig::parseMemoryConfig(const YAML::Node& node) {
    if (!node) return;
    
    if (node["host_memory_pool_size"]) {
        memory_config_.host_memory_pool_size = node["host_memory_pool_size"].as<size_t>();
    }
    if (node["device_memory_pool_size"]) {
        memory_config_.device_memory_pool_size = node["device_memory_pool_size"].as<size_t>();
    }
    if (node["host_memory_blocks"]) {
        memory_config_.host_memory_blocks = node["host_memory_blocks"].as<int>();
    }
    if (node["device_memory_blocks"]) {
        memory_config_.device_memory_blocks = node["device_memory_blocks"].as<int>();
    }
    if (node["cuda_stream_pool_size"]) {
        memory_config_.cuda_stream_pool_size = node["cuda_stream_pool_size"].as<int>();
    }
}

}} // namespace urology::config 