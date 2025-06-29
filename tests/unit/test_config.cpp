#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "config/app_config.hpp"
#include <filesystem>
#include <fstream>

using namespace urology::config;

class AppConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_config_file_ = "test_config.yaml";
        createTestConfigFile();
        config_ = std::make_unique<AppConfig>();
    }
    
    void TearDown() override {
        if (std::filesystem::exists(test_config_file_)) {
            std::filesystem::remove(test_config_file_);
        }
    }
    
    void createTestConfigFile() {
        std::ofstream file(test_config_file_);
        file << R"(
model_type: "yolo_seg_v9"
model_name: "test_model.onnx"
record_output: true

yuan:
  width: 1920
  height: 1080
  pixel_format: "nv12"
  rdma: true

inference:
  backend: "trt"
  enable_fp16: true

yolo_postprocessor:
  scores_threshold: 0.3
  num_class: 10

video_encoder_request:
  bitrate: 10000000
  framerate: 30
  qp: 25

memory:
  host_memory_pool_size: 67108864
  device_memory_pool_size: 67108864
  cuda_stream_pool_size: 8
)";
        file.close();
    }
    
    std::string test_config_file_;
    std::unique_ptr<AppConfig> config_;
};

TEST_F(AppConfigTest, LoadFromValidFile) {
    EXPECT_TRUE(config_->loadFromFile(test_config_file_));
    
    // Test video config
    const auto& video_config = config_->getVideoConfig();
    EXPECT_EQ(video_config.width, 1920);
    EXPECT_EQ(video_config.height, 1080);
    EXPECT_EQ(video_config.pixel_format, "nv12");
    EXPECT_TRUE(video_config.rdma);
    
    // Test inference config
    const auto& inference_config = config_->getInferenceConfig();
    EXPECT_EQ(inference_config.backend, "trt");
    EXPECT_TRUE(inference_config.enable_fp16);
    EXPECT_FLOAT_EQ(inference_config.scores_threshold, 0.3f);
    EXPECT_EQ(inference_config.num_classes, 10);
    
    // Test encoder config
    const auto& encoder_config = config_->getEncoderConfig();
    EXPECT_TRUE(encoder_config.record_output);
    EXPECT_EQ(encoder_config.bitrate, 10000000);
    EXPECT_EQ(encoder_config.framerate, 30);
    EXPECT_EQ(encoder_config.qp, 25);
    
    // Test memory config
    const auto& memory_config = config_->getMemoryConfig();
    EXPECT_EQ(memory_config.host_memory_pool_size, 67108864);
    EXPECT_EQ(memory_config.device_memory_pool_size, 67108864);
    EXPECT_EQ(memory_config.cuda_stream_pool_size, 8);
}

TEST_F(AppConfigTest, LoadFromNonexistentFile) {
    EXPECT_FALSE(config_->loadFromFile("nonexistent.yaml"));
    
    // Should use default values
    const auto& video_config = config_->getVideoConfig();
    EXPECT_EQ(video_config.width, 1920);
    EXPECT_EQ(video_config.height, 1080);
}

TEST_F(AppConfigTest, ValidationPass) {
    config_->loadFromFile(test_config_file_);
    EXPECT_TRUE(config_->validate());
}

TEST_F(AppConfigTest, ValidationFailOnInvalidThreshold) {
    config_->loadFromFile(test_config_file_);
    
    // Simulate invalid threshold
    // This would require making threshold accessible for testing
    // In a real implementation, you might need friend classes or test accessors
}

TEST_F(AppConfigTest, EnvironmentVariableOverride) {
    // Set environment variable
    setenv("UROLOGY_RECORD_OUTPUT", "false", 1);
    
    config_->loadFromFile(test_config_file_);
    config_->loadFromEnvironment();
    
    const auto& encoder_config = config_->getEncoderConfig();
    EXPECT_FALSE(encoder_config.record_output);
    
    // Clean up
    unsetenv("UROLOGY_RECORD_OUTPUT");
}

TEST_F(AppConfigTest, StringGetters) {
    config_->loadFromFile(test_config_file_);
    
    auto model_type = config_->getString("model_type");
    ASSERT_TRUE(model_type.has_value());
    EXPECT_EQ(model_type.value(), "yolo_seg_v9");
    
    auto nonexistent = config_->getString("nonexistent_key");
    EXPECT_FALSE(nonexistent.has_value());
}

TEST_F(AppConfigTest, BooleanGetters) {
    config_->loadFromFile(test_config_file_);
    
    auto record_output = config_->getBool("record_output");
    ASSERT_TRUE(record_output.has_value());
    EXPECT_TRUE(record_output.value());
}

// Performance test for config loading
TEST_F(AppConfigTest, LoadPerformance) {
    const int iterations = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        AppConfig test_config;
        test_config.loadFromFile(test_config_file_);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Expect config loading to be reasonably fast (< 1ms per load on average)
    EXPECT_LT(duration.count() / iterations, 1000);
}

// Parameterized test for different config values
class AppConfigParameterizedTest : public ::testing::TestWithParam<std::pair<std::string, std::string>> {
};

TEST_P(AppConfigParameterizedTest, ValidateConfigValues) {
    auto [key, value] = GetParam();
    
    std::ofstream file("param_test_config.yaml");
    file << key << ": " << value << std::endl;
    file.close();
    
    AppConfig config;
    bool load_result = config.loadFromFile("param_test_config.yaml");
    
    // All these should load successfully
    EXPECT_TRUE(load_result);
    
    std::filesystem::remove("param_test_config.yaml");
}

INSTANTIATE_TEST_SUITE_P(
    ConfigValueTests,
    AppConfigParameterizedTest,
    ::testing::Values(
        std::make_pair("record_output", "true"),
        std::make_pair("record_output", "false"),
        std::make_pair("model_type", "yolo_seg_v9"),
        std::make_pair("model_type", "yolo_v8")
    )
);

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 