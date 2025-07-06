#pragma once

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>
#include <memory>
#include <functional>
#include <thread>

namespace urology {
namespace utils {

/**
 * @brief Performance metrics structure
 */
struct PerformanceMetrics {
    std::string name;
    size_t call_count = 0;
    double total_time_ms = 0.0;
    double min_time_ms = std::numeric_limits<double>::max();
    double max_time_ms = 0.0;
    double avg_time_ms = 0.0;
    double last_time_ms = 0.0;
    std::chrono::steady_clock::time_point last_update;
    
    void update(double elapsed_ms);
    void reset();
};

/**
 * @brief GPU performance metrics
 */
struct GPUMetrics {
    float gpu_utilization = 0.0f;
    size_t gpu_memory_used = 0;
    size_t gpu_memory_total = 0;
    float gpu_temperature = 0.0f;
    size_t vram_used = 0;
    size_t vram_total = 0;
    
    void update();
    bool isAvailable() const;
};

/**
 * @brief System resource metrics
 */
struct SystemMetrics {
    float cpu_usage = 0.0f;
    size_t memory_used = 0;
    size_t memory_total = 0;
    float memory_usage_percent = 0.0f;
    size_t disk_io_read = 0;
    size_t disk_io_write = 0;
    
    void update();
};

/**
 * @brief High-performance monitoring system
 */
class PerformanceMonitor {
public:
    /**
     * @brief Get the global performance monitor instance
     */
    static PerformanceMonitor& getInstance();
    
    /**
     * @brief Start monitoring a function/operation
     */
    void startTimer(const std::string& name);
    
    /**
     * @brief Stop monitoring a function/operation
     */
    void stopTimer(const std::string& name);
    
    /**
     * @brief Record a custom metric value
     */
    void recordMetric(const std::string& name, double value_ms);
    
    /**
     * @brief Get performance metrics for a specific operation
     */
    PerformanceMetrics getMetrics(const std::string& name) const;
    
    /**
     * @brief Get all performance metrics
     */
    std::map<std::string, PerformanceMetrics> getAllMetrics() const;
    
    /**
     * @brief Get GPU metrics
     */
    GPUMetrics getGPUMetrics() const;
    
    /**
     * @brief Get system metrics
     */
    SystemMetrics getSystemMetrics() const;
    
    /**
     * @brief Enable/disable automatic system monitoring
     */
    void setSystemMonitoringEnabled(bool enabled);
    
    /**
     * @brief Set monitoring update interval (milliseconds)
     */
    void setUpdateInterval(int interval_ms);
    
    /**
     * @brief Reset all metrics
     */
    void reset();
    
    /**
     * @brief Generate performance report
     */
    std::string generateReport() const;
    
    /**
     * @brief Export metrics to JSON file
     */
    bool exportToJSON(const std::string& filename) const;
    
    /**
     * @brief Set performance warning thresholds
     */
    void setWarningThresholds(const std::string& name, double warning_ms, double critical_ms);
    
    /**
     * @brief Register performance callback
     */
    void registerCallback(const std::string& name, 
                         std::function<void(const PerformanceMetrics&)> callback);

private:
    PerformanceMonitor();
    ~PerformanceMonitor();
    
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;
    
    void monitorSystemResources();
    void checkThresholds(const std::string& name, const PerformanceMetrics& metrics);
    
    mutable std::mutex metrics_mutex_;
    std::map<std::string, PerformanceMetrics> metrics_;
    std::map<std::string, std::chrono::steady_clock::time_point> active_timers_;
    
    mutable std::mutex system_mutex_;
    GPUMetrics gpu_metrics_;
    SystemMetrics system_metrics_;
    
    std::atomic<bool> system_monitoring_enabled_{false};
    std::atomic<bool> monitoring_thread_running_{false};
    std::unique_ptr<std::thread> monitoring_thread_;
    int update_interval_ms_ = 1000;
    
    struct ThresholdConfig {
        double warning_ms = 0.0;
        double critical_ms = 0.0;
    };
    std::map<std::string, ThresholdConfig> thresholds_;
    std::map<std::string, std::function<void(const PerformanceMetrics&)>> callbacks_;
};

/**
 * @brief RAII performance timer with automatic monitoring
 */
class ScopedPerformanceTimer {
public:
    explicit ScopedPerformanceTimer(const std::string& name);
    ~ScopedPerformanceTimer();
    
    /**
     * @brief Get elapsed time so far
     */
    double getElapsedMs() const;

private:
    std::string name_;
    std::chrono::steady_clock::time_point start_time_;
};

/**
 * @brief Frame rate calculator for video processing
 */
class FrameRateCalculator {
public:
    explicit FrameRateCalculator(size_t window_size = 30);
    
    /**
     * @brief Record a new frame
     */
    void recordFrame();
    
    /**
     * @brief Get current FPS
     */
    double getCurrentFPS() const;
    
    /**
     * @brief Get average FPS over the window
     */
    double getAverageFPS() const;
    
    /**
     * @brief Reset statistics
     */
    void reset();

private:
    size_t window_size_;
    std::vector<std::chrono::steady_clock::time_point> frame_times_;
    size_t current_index_ = 0;
    bool buffer_full_ = false;
    mutable std::mutex mutex_;
};

/**
 * @brief Pipeline performance analyzer
 */
class PipelineAnalyzer {
public:
    struct StageMetrics {
        std::string name;
        double processing_time_ms = 0.0;
        double queue_time_ms = 0.0;
        size_t queue_size = 0;
        double throughput_fps = 0.0;
    };
    
    /**
     * @brief Record stage start
     */
    void recordStageStart(const std::string& stage_name);
    
    /**
     * @brief Record stage completion
     */
    void recordStageComplete(const std::string& stage_name);
    
    /**
     * @brief Record queue statistics
     */
    void recordQueueStats(const std::string& stage_name, size_t queue_size, double queue_time_ms);
    
    /**
     * @brief Get pipeline bottleneck analysis
     */
    std::vector<StageMetrics> analyzeBottlenecks() const;
    
    /**
     * @brief Generate pipeline performance report
     */
    std::string generatePipelineReport() const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, StageMetrics> stage_metrics_;
    std::map<std::string, std::chrono::steady_clock::time_point> stage_start_times_;
};

}} // namespace urology::utils

// Convenient performance monitoring macros
#define PERF_MONITOR(name) urology::utils::ScopedPerformanceTimer perf_timer(name)
#define PERF_START(name) urology::utils::PerformanceMonitor::getInstance().startTimer(name)
#define PERF_STOP(name) urology::utils::PerformanceMonitor::getInstance().stopTimer(name)
#define PERF_RECORD(name, value) urology::utils::PerformanceMonitor::getInstance().recordMetric(name, value) 