#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <yaml-cpp/yaml.h>

namespace urology {
namespace config {

/**
 * @brief Application configuration management class
 * 
 * This class provides centralized configuration management with validation,
 * default values, and environment variable support.
 */
class AppConfig {
public:
    struct VideoConfig {
        int width = 1920;
        int height = 1080;
        std::string pixel_format = "nv12";
        bool rdma = true;
        std::string input_type = "hdmi";
    };
    
    struct InferenceConfig {
        std::string backend = "trt";
        bool enable_fp16 = true;
        std::string model_type = "yolo_seg_v9";
        std::string model_name = "Urology_yolov11x-seg_3-13-16-17val640rezize_1_4.40.0_nms.onnx";
        float scores_threshold = 0.2f;
        int num_classes = 12;
    };
    
    struct EncoderConfig {
        bool record_output = true;
        int inbuf_storage_type = 1;
        int codec = 0;
        int input_width = 1920;
        int input_height = 1080;
        std::string input_format = "yuv420planar";
        int profile = 2;
        int bitrate = 20000000;
        int framerate = 30;
        std::string config = "pframe_cqp";
        int rate_control_mode = 0;
        int qp = 20;
        int iframe_interval = 5;
    };
    
    struct MemoryConfig {
        size_t host_memory_pool_size = 32 * 1024 * 1024;
        size_t device_memory_pool_size = 32 * 1024 * 1024;
        int host_memory_blocks = 2;
        int device_memory_blocks = 2;
        int cuda_stream_pool_size = 5;
    };

public:
    /**
     * @brief Load configuration from file
     * @param config_file Path to YAML configuration file
     * @return true if loaded successfully, false otherwise
     */
    bool loadFromFile(const std::string& config_file);
    
    /**
     * @brief Load configuration from environment variables
     */
    void loadFromEnvironment();
    
    /**
     * @brief Validate configuration values
     * @return true if configuration is valid, false otherwise
     */
    bool validate() const;
    
    /**
     * @brief Get string value with optional fallback
     */
    std::optional<std::string> getString(const std::string& key) const;
    
    /**
     * @brief Get integer value with optional fallback
     */
    std::optional<int> getInt(const std::string& key) const;
    
    /**
     * @brief Get boolean value with optional fallback
     */
    std::optional<bool> getBool(const std::string& key) const;
    
    // Getters for configuration sections
    const VideoConfig& getVideoConfig() const { return video_config_; }
    const InferenceConfig& getInferenceConfig() const { return inference_config_; }
    const EncoderConfig& getEncoderConfig() const { return encoder_config_; }
    const MemoryConfig& getMemoryConfig() const { return memory_config_; }
    
    // Data paths
    const std::string& getDataPath() const { return data_path_; }
    const std::string& getModelPath() const { return model_path_; }
    const std::string& getOutputPath() const { return output_path_; }
    const std::string& getLabelsFile() const { return labels_file_; }
    
    void setDataPath(const std::string& path) { data_path_ = path; }
    void setModelPath(const std::string& path) { model_path_ = path; }
    void setOutputPath(const std::string& path) { output_path_ = path; }
    void setLabelsFile(const std::string& file) { labels_file_ = file; }

private:
    YAML::Node config_;
    VideoConfig video_config_;
    InferenceConfig inference_config_;
    EncoderConfig encoder_config_;
    MemoryConfig memory_config_;
    
    std::string data_path_;
    std::string model_path_;
    std::string output_path_;
    std::string labels_file_;
    
    void setDefaults();
    void parseVideoConfig(const YAML::Node& node);
    void parseInferenceConfig(const YAML::Node& node);
    void parseEncoderConfig(const YAML::Node& node);
    void parseMemoryConfig(const YAML::Node& node);
};

}} // namespace urology::config 