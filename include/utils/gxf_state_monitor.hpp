/**
 * @file gxf_state_monitor.hpp
 * @brief GXF State Monitor for tracking entity states in Holoscan applications
 */

#ifndef UROLOGY_UTILS_GXF_STATE_MONITOR_HPP
#define UROLOGY_UTILS_GXF_STATE_MONITOR_HPP

#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <numeric>

namespace urology::utils {

/**
 * @brief Entity state enumeration
 */
enum class EntityState {
    NEVER = 0,
    WAIT = 1,
    READY = 2,
    EXECUTE = 3
};

/**
 * @brief Entity state record
 */
struct EntityStateRecord {
    std::string operator_name;
    EntityState state;
    std::chrono::steady_clock::time_point timestamp;
};

/**
 * @brief Operator statistics
 */
struct OperatorStats {
    std::map<EntityState, int> state_counts = {
        {EntityState::NEVER, 0},
        {EntityState::WAIT, 0},
        {EntityState::READY, 0},
        {EntityState::EXECUTE, 0}
    };
    std::vector<double> processing_times;
    std::chrono::steady_clock::time_point last_seen;
    std::chrono::steady_clock::time_point last_wait_time;
};

/**
 * @brief GXF State Monitor class
 * 
 * This class monitors the state of GXF entities in a Holoscan application
 * and provides insights into performance and potential issues.
 */
class GXFStateMonitor {
public:
    /**
     * @brief Constructor
     */
    GXFStateMonitor();

    /**
     * @brief Start monitoring entity states
     */
    void start_monitoring();

    /**
     * @brief Stop monitoring entity states
     */
    void stop_monitoring();

    /**
     * @brief Record entity state change
     * @param operator_name Name of the operator
     * @param state New state of the entity
     */
    void record_entity_state(const std::string& operator_name, EntityState state);

    /**
     * @brief Record processing time for an operator
     * @param operator_name Name of the operator
     * @param time_ms Processing time in milliseconds
     */
    void record_processing_time(const std::string& operator_name, double time_ms);

    /**
     * @brief Get state report as string
     * @return State report string
     */
    std::string get_state_report() const;

private:
    /**
     * @brief Detect state anomalies
     * @param operator_name Name of the operator
     * @param state Current state
     */
    void detect_state_anomalies(const std::string& operator_name, EntityState state);

    /**
     * @brief Print monitoring summary
     */
    void print_summary() const;

    bool monitoring_;
    std::chrono::steady_clock::time_point start_time_;
    std::vector<EntityStateRecord> state_history_;
    std::unordered_map<std::string, OperatorStats> operator_stats_;
    mutable std::mutex mutex_;
};

} // namespace urology::utils

#endif // UROLOGY_UTILS_GXF_STATE_MONITOR_HPP 