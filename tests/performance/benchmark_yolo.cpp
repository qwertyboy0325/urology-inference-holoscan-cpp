#include <benchmark/benchmark.h>
#include <opencv2/opencv.hpp>
#include "operators/yolo_seg_postprocessor.hpp"
#include "utils/yolo_utils.hpp"
#include "utils/performance_monitor.hpp"
#include <random>

using namespace urology;
using namespace urology::utils;

class YoloBenchmark : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Setup test data
        width_ = 640;
        height_ = 640;
        num_classes_ = 12;
        
        // Create synthetic input data
        createSyntheticData();
        
        // Initialize performance monitor
        PerformanceMonitor::getInstance().setSystemMonitoringEnabled(true);
    }
    
    void TearDown(const ::benchmark::State& state) override {
        PerformanceMonitor::getInstance().setSystemMonitoringEnabled(false);
    }

protected:
    void createSyntheticData() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(0.0f, 1.0f);
        
        // Create synthetic detection output
        int output_size = width_ * height_ * (num_classes_ + 5); // YOLO format
        synthetic_output_.resize(output_size);
        
        for (int i = 0; i < output_size; ++i) {
            synthetic_output_[i] = dis(gen);
        }
        
        // Create synthetic image
        synthetic_image_ = cv::Mat::zeros(height_, width_, CV_8UC3);
        cv::randu(synthetic_image_, cv::Scalar(0, 0, 0), cv::Scalar(255, 255, 255));
    }
    
    int width_, height_, num_classes_;
    std::vector<float> synthetic_output_;
    cv::Mat synthetic_image_;
};

// Benchmark YOLO postprocessing
BENCHMARK_DEFINE_F(YoloBenchmark, PostProcessing)(benchmark::State& state) {
    float threshold = 0.5f;
    
    for (auto _ : state) {
        PERF_MONITOR("yolo_postprocessing");
        
        // Simulate YOLO postprocessing
        std::vector<cv::Rect> boxes;
        std::vector<float> confidences;
        std::vector<int> class_ids;
        
        // Parse YOLO output (simplified)
        int grid_size = 80; // 640/8 for stride 8
        int anchors_per_cell = 3;
        
        for (int i = 0; i < grid_size; ++i) {
            for (int j = 0; j < grid_size; ++j) {
                for (int a = 0; a < anchors_per_cell; ++a) {
                    int index = (i * grid_size + j) * anchors_per_cell + a;
                    if (index < synthetic_output_.size() - 5) {
                        float confidence = synthetic_output_[index + 4];
                        
                        if (confidence > threshold) {
                            float x = synthetic_output_[index] * width_;
                            float y = synthetic_output_[index + 1] * height_;
                            float w = synthetic_output_[index + 2] * width_;
                            float h = synthetic_output_[index + 3] * height_;
                            
                            boxes.emplace_back(static_cast<int>(x - w/2), 
                                             static_cast<int>(y - h/2),
                                             static_cast<int>(w), 
                                             static_cast<int>(h));
                            confidences.push_back(confidence);
                            
                            // Find max class
                            int max_class = 0;
                            float max_prob = 0.0f;
                            for (int c = 0; c < num_classes_; ++c) {
                                if (index + 5 + c < synthetic_output_.size()) {
                                    float prob = synthetic_output_[index + 5 + c];
                                    if (prob > max_prob) {
                                        max_prob = prob;
                                        max_class = c;
                                    }
                                }
                            }
                            class_ids.push_back(max_class);
                        }
                    }
                }
            }
        }
        
        // Apply NMS
        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, confidences, threshold, 0.4f, indices);
        
        // Don't let compiler optimize away
        benchmark::DoNotOptimize(indices);
    }
    
    // Set benchmark statistics
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * synthetic_output_.size() * sizeof(float));
}

// Benchmark with different thresholds
BENCHMARK_REGISTER_F(YoloBenchmark, PostProcessing)
    ->Arg(0.1)->Arg(0.3)->Arg(0.5)->Arg(0.7)->Arg(0.9)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// Benchmark image preprocessing
BENCHMARK_DEFINE_F(YoloBenchmark, ImagePreprocessing)(benchmark::State& state) {
    cv::Size target_size(640, 640);
    
    for (auto _ : state) {
        PERF_MONITOR("image_preprocessing");
        
        cv::Mat resized, normalized;
        
        // Resize
        cv::resize(synthetic_image_, resized, target_size);
        
        // Normalize to [0, 1]
        resized.convertTo(normalized, CV_32F, 1.0/255.0);
        
        // Convert to RGB (from BGR)
        cv::cvtColor(normalized, normalized, cv::COLOR_BGR2RGB);
        
        benchmark::DoNotOptimize(normalized);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * synthetic_image_.total() * synthetic_image_.elemSize());
}

BENCHMARK_REGISTER_F(YoloBenchmark, ImagePreprocessing)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// Memory allocation benchmark
BENCHMARK_DEFINE_F(YoloBenchmark, MemoryAllocation)(benchmark::State& state) {
    size_t allocation_size = state.range(0);
    
    for (auto _ : state) {
        PERF_MONITOR("memory_allocation");
        
        // Test different allocation strategies
        std::vector<float> data(allocation_size);
        std::fill(data.begin(), data.end(), 1.0f);
        
        benchmark::DoNotOptimize(data);
    }
    
    state.SetBytesProcessed(state.iterations() * allocation_size * sizeof(float));
}

BENCHMARK_REGISTER_F(YoloBenchmark, MemoryAllocation)
    ->RangeMultiplier(2)
    ->Range(1024, 1024*1024*8) // 1KB to 8MB
    ->Unit(benchmark::kMicrosecond);

// Multithreaded processing benchmark
static void BM_ParallelProcessing(benchmark::State& state) {
    int num_threads = state.range(0);
    int work_items = 1000;
    
    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::atomic<int> completed(0);
        
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                int start = (work_items * t) / num_threads;
                int end = (work_items * (t + 1)) / num_threads;
                
                for (int i = start; i < end; ++i) {
                    // Simulate work
                    volatile float result = std::sin(i * 0.001f);
                    benchmark::DoNotOptimize(result);
                }
                completed.fetch_add(end - start);
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        benchmark::DoNotOptimize(completed.load());
    }
    
    state.SetItemsProcessed(state.iterations() * work_items);
}

BENCHMARK(BM_ParallelProcessing)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// GPU vs CPU comparison (if CUDA available)
#ifdef CUDA_FOUND
static void BM_MatrixMultiplyCPU(benchmark::State& state) {
    int size = state.range(0);
    cv::Mat a = cv::Mat::ones(size, size, CV_32F);
    cv::Mat b = cv::Mat::ones(size, size, CV_32F);
    cv::Mat c;
    
    for (auto _ : state) {
        c = a * b;
        benchmark::DoNotOptimize(c);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * size * size * sizeof(float) * 3);
}

BENCHMARK(BM_MatrixMultiplyCPU)
    ->Arg(64)->Arg(128)->Arg(256)->Arg(512)
    ->Unit(benchmark::kMillisecond);
#endif

// Custom counter for FPS measurement
static void BM_VideoProcessingFPS(benchmark::State& state) {
    cv::Mat frame = cv::Mat::zeros(640, 480, CV_8UC3);
    int frame_count = 0;
    
    for (auto _ : state) {
        // Simulate video frame processing
        cv::Mat processed;
        cv::GaussianBlur(frame, processed, cv::Size(15, 15), 0);
        cv::Canny(processed, processed, 100, 200);
        
        frame_count++;
        benchmark::DoNotOptimize(processed);
    }
    
    // Calculate FPS
    double elapsed_seconds = static_cast<double>(state.iterations()) / 
                           static_cast<double>(benchmark::internal::GetTimerResolution());
    double fps = frame_count / elapsed_seconds;
    
    state.counters["FPS"] = fps;
    state.counters["Frames"] = frame_count;
}

BENCHMARK(BM_VideoProcessingFPS)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// Memory bandwidth test
static void BM_MemoryBandwidth(benchmark::State& state) {
    size_t size = state.range(0);
    std::vector<char> src(size, 'A');
    std::vector<char> dst(size);
    
    for (auto _ : state) {
        std::memcpy(dst.data(), src.data(), size);
        benchmark::DoNotOptimize(dst);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
    
    // Calculate bandwidth in MB/s
    double bytes_per_second = static_cast<double>(state.iterations() * size) / 
                             (static_cast<double>(state.elapsed_time()) / 1e9);
    state.counters["Bandwidth_MB/s"] = bytes_per_second / (1024 * 1024);
}

BENCHMARK(BM_MemoryBandwidth)
    ->RangeMultiplier(2)
    ->Range(1024, 64*1024*1024) // 1KB to 64MB
    ->Unit(benchmark::kMicrosecond);

// Performance regression test
static void BM_RegressionTest(benchmark::State& state) {
    // This benchmark should maintain consistent performance
    // Useful for detecting performance regressions in CI/CD
    
    const int work_size = 10000;
    std::vector<float> data(work_size);
    
    for (auto _ : state) {
        // Standard mathematical operations
        for (int i = 0; i < work_size; ++i) {
            data[i] = std::sin(i * 0.001f) + std::cos(i * 0.002f);
        }
        
        // Sorting
        std::sort(data.begin(), data.end());
        
        benchmark::DoNotOptimize(data);
    }
    
    state.SetItemsProcessed(state.iterations() * work_size);
}

BENCHMARK(BM_RegressionTest)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000);

int main(int argc, char** argv) {
    // Initialize performance monitoring
    PerformanceMonitor::getInstance().initialize("benchmark_results.log");
    
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    
    ::benchmark::RunSpecifiedBenchmarks();
    
    // Generate performance report
    auto report = PerformanceMonitor::getInstance().generateReport();
    std::cout << "\n=== Performance Monitor Report ===\n" << report << std::endl;
    
    return 0;
} 