#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace urology {
namespace utils {

/**
 * @brief Log levels for filtering messages
 */
enum class UrologyLogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

/**
 * @brief High-performance, thread-safe logger with multiple outputs
 */
class UrologyLogger {
public:
    /**
     * @brief Get the global logger instance
     */
    static UrologyLogger& getInstance();
    
    /**
     * @brief Initialize logger with configuration
     * @param log_file Path to log file (empty for no file logging)
     * @param level Minimum log level to output
     * @param console_output Enable console output
     */
    void initialize(const std::string& log_file = "", 
                   UrologyLogLevel level = UrologyLogLevel::INFO,
                   bool console_output = true);
    
    /**
     * @brief Log a message with specified level
     */
    void log(UrologyLogLevel level, const std::string& message, 
             const char* file = __builtin_FILE(), 
             int line = __builtin_LINE(),
             const char* func = __builtin_FUNCTION());
    
    /**
     * @brief Set minimum log level
     */
    void setLogLevel(UrologyLogLevel level) { min_level_ = level; }
    
    /**
     * @brief Get current log level
     */
    UrologyLogLevel getLogLevel() const { return min_level_; }
    
    /**
     * @brief Enable/disable console output
     */
    void setConsoleOutput(bool enable) { console_output_ = enable; }
    
    /**
     * @brief Enable/disable file output
     */
    void setFileOutput(bool enable) { file_output_ = enable; }
    
    /**
     * @brief Flush all pending log messages
     */
    void flush();
    
    /**
     * @brief Check if a log level is enabled
     */
    bool isLevelEnabled(UrologyLogLevel level) const {
        return level >= min_level_;
    }

private:
    UrologyLogger() = default;
    ~UrologyLogger();
    
    UrologyLogger(const UrologyLogger&) = delete;
    UrologyLogger& operator=(const UrologyLogger&) = delete;
    
    std::string formatMessage(UrologyLogLevel level, const std::string& message,
                             const char* file, int line, const char* func);
    std::string getCurrentTime();
    std::string getLogLevelString(UrologyLogLevel level);
    
    std::ofstream file_stream_;
    std::mutex mutex_;
    UrologyLogLevel min_level_ = UrologyLogLevel::INFO;
    bool console_output_ = true;
    bool file_output_ = false;
    bool initialized_ = false;
};

/**
 * @brief RAII performance timer for logging function execution times
 */
class UrologyPerformanceTimer {
public:
    explicit UrologyPerformanceTimer(const std::string& name, UrologyLogLevel level = UrologyLogLevel::DEBUG);
    ~UrologyPerformanceTimer();
    
    /**
     * @brief Get elapsed time in milliseconds
     */
    double getElapsedMs() const;
    
private:
    std::string name_;
    UrologyLogLevel level_;
    std::chrono::high_resolution_clock::time_point start_time_;
};

}} // namespace urology::utils

// Convenient logging macros with UROLOGY_ prefix to avoid conflicts
#define UROLOGY_LOG_TRACE(msg) \
    do { \
        if (urology::utils::UrologyLogger::getInstance().isLevelEnabled(urology::utils::UrologyLogLevel::TRACE)) { \
            std::ostringstream oss; oss << msg; \
            urology::utils::UrologyLogger::getInstance().log(urology::utils::UrologyLogLevel::TRACE, oss.str(), __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)

#define UROLOGY_LOG_DEBUG(msg) \
    do { \
        if (urology::utils::UrologyLogger::getInstance().isLevelEnabled(urology::utils::UrologyLogLevel::DEBUG)) { \
            std::ostringstream oss; oss << msg; \
            urology::utils::UrologyLogger::getInstance().log(urology::utils::UrologyLogLevel::DEBUG, oss.str(), __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)

#define UROLOGY_LOG_INFO(msg) \
    do { \
        if (urology::utils::UrologyLogger::getInstance().isLevelEnabled(urology::utils::UrologyLogLevel::INFO)) { \
            std::ostringstream oss; oss << msg; \
            urology::utils::UrologyLogger::getInstance().log(urology::utils::UrologyLogLevel::INFO, oss.str(), __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)

#define UROLOGY_LOG_WARN(msg) \
    do { \
        if (urology::utils::UrologyLogger::getInstance().isLevelEnabled(urology::utils::UrologyLogLevel::WARN)) { \
            std::ostringstream oss; oss << msg; \
            urology::utils::UrologyLogger::getInstance().log(urology::utils::UrologyLogLevel::WARN, oss.str(), __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)

#define UROLOGY_LOG_ERROR(msg) \
    do { \
        if (urology::utils::UrologyLogger::getInstance().isLevelEnabled(urology::utils::UrologyLogLevel::ERROR)) { \
            std::ostringstream oss; oss << msg; \
            urology::utils::UrologyLogger::getInstance().log(urology::utils::UrologyLogLevel::ERROR, oss.str(), __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)

#define UROLOGY_LOG_FATAL(msg) \
    do { \
        std::ostringstream oss; oss << msg; \
        urology::utils::UrologyLogger::getInstance().log(urology::utils::UrologyLogLevel::FATAL, oss.str(), __FILE__, __LINE__, __FUNCTION__); \
    } while(0)

// Performance timing macros
#define UROLOGY_PERF_TIMER(name) urology::utils::UrologyPerformanceTimer timer(name)
#define UROLOGY_PERF_TIMER_LEVEL(name, level) urology::utils::UrologyPerformanceTimer timer(name, level) 