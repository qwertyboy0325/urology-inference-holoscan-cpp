#include "utils/gxf_state_monitor.hpp"

namespace urology::utils {

GXFStateMonitor::GXFStateMonitor() : monitoring_(false) {}

void GXFStateMonitor::start_monitoring() {
    monitoring_ = true;
    start_time_ = std::chrono::steady_clock::now();
    std::cout << "GXF State Monitor: Started monitoring" << std::endl;
}

void GXFStateMonitor::stop_monitoring() {
    monitoring_ = false;
    std::cout << "GXF State Monitor: Stopped monitoring" << std::endl;
    print_summary();
}

void GXFStateMonitor::record_entity_state(const std::string& operator_name, 
                                          EntityState state) {
    if (!monitoring_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::steady_clock::now();
    EntityStateRecord record{operator_name, state, now};
    
    state_history_.push_back(record);
    operator_stats_[operator_name].state_counts[state]++;
    operator_stats_[operator_name].last_seen = now;
    
    // 檢測狀態異常
    detect_state_anomalies(operator_name, state);
}

void GXFStateMonitor::record_processing_time(const std::string& operator_name, 
                                            double time_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    operator_stats_[operator_name].processing_times.push_back(time_ms);
}

void GXFStateMonitor::detect_state_anomalies(const std::string& operator_name, 
                                            EntityState state) {
    auto& stats = operator_stats_[operator_name];
    
    // 檢測長時間卡在 WAIT 狀態
    if (state == EntityState::WAIT) {
        auto now = std::chrono::steady_clock::now();
        if (stats.last_wait_time != std::chrono::steady_clock::time_point{}) {
            auto wait_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - stats.last_wait_time).count();
            
            if (wait_duration > 1000) { // 1秒以上
                std::cout << "GXF State Monitor: " << operator_name << " stuck in WAIT for " << wait_duration << "ms" << std::endl;
            }
        }
        stats.last_wait_time = now;
    } else {
        stats.last_wait_time = std::chrono::steady_clock::time_point{};
    }
    
    // 檢測 NEVER 狀態（可能的問題）
    if (state == EntityState::NEVER) {
        std::cout << "GXF State Monitor: " << operator_name << " entered NEVER state - possible deadlock" << std::endl;
    }
}

void GXFStateMonitor::print_summary() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto total_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start_time_).count();
    
    std::cout << "=== GXF State Monitor Summary (" << total_time << "s) ===" << std::endl;
    
    for (const auto& [op_name, stats] : operator_stats_) {
        std::cout << "Operator: " << op_name << std::endl;
        std::cout << "  NEVER: " << stats.state_counts.at(EntityState::NEVER) << std::endl;
        std::cout << "  WAIT: " << stats.state_counts.at(EntityState::WAIT) << std::endl;
        std::cout << "  READY: " << stats.state_counts.at(EntityState::READY) << std::endl;
        std::cout << "  EXECUTE: " << stats.state_counts.at(EntityState::EXECUTE) << std::endl;
        
        if (!stats.processing_times.empty()) {
            double avg_time = std::accumulate(stats.processing_times.begin(), 
                                            stats.processing_times.end(), 0.0) 
                            / stats.processing_times.size();
            std::cout << "  Avg Processing Time: " << avg_time << "ms" << std::endl;
        }
    }
}

std::string GXFStateMonitor::get_state_report() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    oss << "GXF Entity States:\n";
    for (const auto& [op_name, stats] : operator_stats_) {
        oss << op_name << ": ";
        oss << "NEVER=" << stats.state_counts.at(EntityState::NEVER) << " ";
        oss << "WAIT=" << stats.state_counts.at(EntityState::WAIT) << " ";
        oss << "READY=" << stats.state_counts.at(EntityState::READY) << " ";
        oss << "EXECUTE=" << stats.state_counts.at(EntityState::EXECUTE) << "\n";
    }
    
    return oss.str();
}

} // namespace urology::utils 