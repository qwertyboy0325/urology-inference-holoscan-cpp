#include "utils/performance_monitor.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <thread>
#include <cstring>
#include <limits>

// For system monitoring
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>

namespace urology {
namespace utils {

// PerformanceMetrics implementation
void PerformanceMetrics::update(double elapsed_ms) {
    call_count++;
    total_time_ms += elapsed_ms;
    min_time_ms = std::min(min_time_ms, elapsed_ms);
    max_time_ms = std::max(max_time_ms, elapsed_ms);
    avg_time_ms = total_time_ms / call_count;
    last_time_ms = elapsed_ms;
    last_update = std::chrono::steady_clock::now();
}

void PerformanceMetrics::reset() {
    call_count = 0;
    total_time_ms = 0.0;
    min_time_ms = std::numeric_limits<double>::max();
    max_time_ms = 0.0;
    avg_time_ms = 0.0;
    last_time_ms = 0.0;
    last_update = std::chrono::steady_clock::now();
}

// GPUMetrics implementation
void GPUMetrics::update() {
    // Basic implementation - would need CUDA/nvidia-ml-py for real GPU monitoring
    gpu_utilization = 0.0f;
    gpu_memory_used = 0;
    gpu_memory_total = 0;
    gpu_temperature = 0.0f;
    vram_used = 0;
    vram_total = 0;
}

bool GPUMetrics::isAvailable() const {
    return false; // Simplified implementation
}

// SystemMetrics implementation
void SystemMetrics::update() {
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        memory_total = info.totalram * info.mem_unit;
        memory_used = memory_total - (info.freeram * info.mem_unit);
        memory_usage_percent = (static_cast<float>(memory_used) / memory_total) * 100.0f;
    }
    
    // Simplified CPU usage (would need /proc/stat parsing for accuracy)
    cpu_usage = 0.0f;
    disk_io_read = 0;
    disk_io_write = 0;
}

// PerformanceMonitor implementation
PerformanceMonitor& PerformanceMonitor::getInstance() {
    static PerformanceMonitor instance;
    return instance;
}

PerformanceMonitor::PerformanceMonitor() {
    // Initialize system monitoring thread if needed
}

PerformanceMonitor::~PerformanceMonitor() {
    monitoring_thread_running_ = false;
    if (monitoring_thread_ && monitoring_thread_->joinable()) {
        monitoring_thread_->join();
    }
}

void PerformanceMonitor::startTimer(const std::string& name) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    active_timers_[name] = std::chrono::steady_clock::now();
}

void PerformanceMonitor::stopTimer(const std::string& name) {
    auto end_time = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = active_timers_.find(name);
    if (it != active_timers_.end()) {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - it->second).count();
        double elapsed_ms = duration / 1000.0;
        
        metrics_[name].update(elapsed_ms);
        active_timers_.erase(it);
        
        checkThresholds(name, metrics_[name]);
    }
}

void PerformanceMonitor::recordMetric(const std::string& name, double value_ms) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_[name].update(value_ms);
    checkThresholds(name, metrics_[name]);
}

PerformanceMetrics PerformanceMonitor::getMetrics(const std::string& name) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto it = metrics_.find(name);
    return (it != metrics_.end()) ? it->second : PerformanceMetrics{};
}

std::map<std::string, PerformanceMetrics> PerformanceMonitor::getAllMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
}

GPUMetrics PerformanceMonitor::getGPUMetrics() const {
    std::lock_guard<std::mutex> lock(system_mutex_);
    return gpu_metrics_;
}

SystemMetrics PerformanceMonitor::getSystemMetrics() const {
    std::lock_guard<std::mutex> lock(system_mutex_);
    return system_metrics_;
}

void PerformanceMonitor::setSystemMonitoringEnabled(bool enabled) {
    system_monitoring_enabled_ = enabled;
    if (enabled && !monitoring_thread_running_) {
        monitoring_thread_running_ = true;
        monitoring_thread_ = std::make_unique<std::thread>(&PerformanceMonitor::monitorSystemResources, this);
    }
}

void PerformanceMonitor::setUpdateInterval(int interval_ms) {
    update_interval_ms_ = interval_ms;
}

void PerformanceMonitor::reset() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_.clear();
    active_timers_.clear();
}

std::string PerformanceMonitor::generateReport() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "=== Performance Report ===\n";
    
    for (const auto& [name, metrics] : metrics_) {
        oss << "Operation: " << name << "\n";
        oss << "  Calls: " << metrics.call_count << "\n";
        oss << "  Total: " << metrics.total_time_ms << " ms\n";
        oss << "  Average: " << metrics.avg_time_ms << " ms\n";
        oss << "  Min: " << metrics.min_time_ms << " ms\n";
        oss << "  Max: " << metrics.max_time_ms << " ms\n";
        oss << "  Last: " << metrics.last_time_ms << " ms\n";
        oss << "\n";
    }
    
    return oss.str();
}

bool PerformanceMonitor::exportToJSON(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << "{\n  \"metrics\": {\n";
    bool first = true;
    for (const auto& [name, metrics] : metrics_) {
        if (!first) file << ",\n";
        file << "    \"" << name << "\": {\n";
        file << "      \"call_count\": " << metrics.call_count << ",\n";
        file << "      \"total_time_ms\": " << metrics.total_time_ms << ",\n";
        file << "      \"avg_time_ms\": " << metrics.avg_time_ms << ",\n";
        file << "      \"min_time_ms\": " << metrics.min_time_ms << ",\n";
        file << "      \"max_time_ms\": " << metrics.max_time_ms << ",\n";
        file << "      \"last_time_ms\": " << metrics.last_time_ms << "\n";
        file << "    }";
        first = false;
    }
    file << "\n  }\n}\n";
    return true;
}

void PerformanceMonitor::setWarningThresholds(const std::string& name, double warning_ms, double critical_ms) {
    ThresholdConfig config;
    config.warning_ms = warning_ms;
    config.critical_ms = critical_ms;
    thresholds_[name] = config;
}

void PerformanceMonitor::registerCallback(const std::string& name, 
                                         std::function<void(const PerformanceMetrics&)> callback) {
    callbacks_[name] = callback;
}

void PerformanceMonitor::monitorSystemResources() {
    while (monitoring_thread_running_) {
        {
            std::lock_guard<std::mutex> lock(system_mutex_);
            system_metrics_.update();
            gpu_metrics_.update();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(update_interval_ms_));
    }
}

void PerformanceMonitor::checkThresholds(const std::string& name, const PerformanceMetrics& metrics) {
    auto threshold_it = thresholds_.find(name);
    if (threshold_it != thresholds_.end()) {
        const auto& config = threshold_it->second;
        if (metrics.last_time_ms > config.critical_ms) {
            LOG_ERROR("Performance critical threshold exceeded for '{}': {:.2f}ms > {:.2f}ms", 
                     name, metrics.last_time_ms, config.critical_ms);
        } else if (metrics.last_time_ms > config.warning_ms) {
            LOG_WARN("Performance warning threshold exceeded for '{}': {:.2f}ms > {:.2f}ms", 
                    name, metrics.last_time_ms, config.warning_ms);
        }
    }
    
    auto callback_it = callbacks_.find(name);
    if (callback_it != callbacks_.end()) {
        callback_it->second(metrics);
    }
}

// ScopedPerformanceTimer implementation
ScopedPerformanceTimer::ScopedPerformanceTimer(const std::string& name) 
    : name_(name), start_time_(std::chrono::steady_clock::now()) {
}

ScopedPerformanceTimer::~ScopedPerformanceTimer() {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time_).count();
    double elapsed_ms = duration / 1000.0;
    
    PerformanceMonitor::getInstance().recordMetric(name_, elapsed_ms);
}

double ScopedPerformanceTimer::getElapsedMs() const {
    auto current_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        current_time - start_time_).count();
    return duration / 1000.0;
}

// FrameRateCalculator implementation
FrameRateCalculator::FrameRateCalculator(size_t window_size) 
    : window_size_(window_size) {
    frame_times_.reserve(window_size);
}

void FrameRateCalculator::recordFrame() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    
    if (frame_times_.size() < window_size_) {
        frame_times_.push_back(now);
    } else {
        frame_times_[current_index_] = now;
        current_index_ = (current_index_ + 1) % window_size_;
        buffer_full_ = true;
    }
}

double FrameRateCalculator::getCurrentFPS() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frame_times_.size() < 2) return 0.0;
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        frame_times_.back() - frame_times_[frame_times_.size() - 2]).count();
    return duration > 0 ? 1000000.0 / duration : 0.0;
}

double FrameRateCalculator::getAverageFPS() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frame_times_.size() < 2) return 0.0;
    
    auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        frame_times_.back() - frame_times_.front()).count();
    
    size_t frame_count = buffer_full_ ? window_size_ - 1 : frame_times_.size() - 1;
    return total_duration > 0 ? (frame_count * 1000000.0) / total_duration : 0.0;
}

void FrameRateCalculator::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    frame_times_.clear();
    current_index_ = 0;
    buffer_full_ = false;
}

// PipelineAnalyzer implementation
void PipelineAnalyzer::recordStageStart(const std::string& stage_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    stage_start_times_[stage_name] = std::chrono::steady_clock::now();
}

void PipelineAnalyzer::recordStageComplete(const std::string& stage_name) {
    auto end_time = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = stage_start_times_.find(stage_name);
    if (it != stage_start_times_.end()) {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - it->second).count();
        stage_metrics_[stage_name].processing_time_ms = duration / 1000.0;
        stage_start_times_.erase(it);
    }
}

void PipelineAnalyzer::recordQueueStats(const std::string& stage_name, size_t queue_size, double queue_time_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    stage_metrics_[stage_name].queue_size = queue_size;
    stage_metrics_[stage_name].queue_time_ms = queue_time_ms;
    
    if (stage_metrics_[stage_name].processing_time_ms > 0) {
        stage_metrics_[stage_name].throughput_fps = 1000.0 / stage_metrics_[stage_name].processing_time_ms;
    }
}

std::vector<PipelineAnalyzer::StageMetrics> PipelineAnalyzer::analyzeBottlenecks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<StageMetrics> sorted_stages;
    
    for (const auto& [name, metrics] : stage_metrics_) {
        sorted_stages.push_back(metrics);
    }
    
    std::sort(sorted_stages.begin(), sorted_stages.end(),
        [](const StageMetrics& a, const StageMetrics& b) {
            return a.processing_time_ms > b.processing_time_ms;
        });
    
    return sorted_stages;
}

std::string PipelineAnalyzer::generatePipelineReport() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "=== Pipeline Analysis Report ===\n";
    
    for (const auto& [name, metrics] : stage_metrics_) {
        oss << "Stage: " << name << "\n";
        oss << "  Processing: " << metrics.processing_time_ms << " ms\n";
        oss << "  Queue Time: " << metrics.queue_time_ms << " ms\n";
        oss << "  Queue Size: " << metrics.queue_size << "\n";
        oss << "  Throughput: " << metrics.throughput_fps << " FPS\n";
        oss << "\n";
    }
    
    return oss.str();
}

}} // namespace urology::utils 