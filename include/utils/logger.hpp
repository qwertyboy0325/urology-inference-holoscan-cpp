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
enum class LogLevel {
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
class Logger {
public:
    /**
     * @brief Get the global logger instance
     */
    static Logger& getInstance();
    
    /**
     * @brief Initialize logger with configuration
     * @param log_file Path to log file (empty for no file logging)
     * @param level Minimum log level to output
     * @param console_output Enable console output
     */
    void initialize(const std::string& log_file = "", 
                   LogLevel level = LogLevel::INFO,
                   bool console_output = true);
    
    /**
     * @brief Log a message with specified level
     */
    void log(LogLevel level, const std::string& message, 
             const char* file = __builtin_FILE(), 
             int line = __builtin_LINE(),
             const char* func = __builtin_FUNCTION());
    
    /**
     * @brief Set minimum log level
     */
    void setLogLevel(LogLevel level) { min_level_ = level; }
    
    /**
     * @brief Get current log level
     */
    LogLevel getLogLevel() const { return min_level_; }
    
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
    bool isLevelEnabled(LogLevel level) const {
        return level >= min_level_;
    }

private:
    Logger() = default;
    ~Logger();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::string formatMessage(LogLevel level, const std::string& message,
                             const char* file, int line, const char* func);
    std::string getCurrentTime();
    std::string getLogLevelString(LogLevel level);
    
    std::ofstream file_stream_;
    std::mutex mutex_;
    LogLevel min_level_ = LogLevel::INFO;
    bool console_output_ = true;
    bool file_output_ = false;
    bool initialized_ = false;
};

/**
 * @brief RAII performance timer for logging function execution times
 */
class PerformanceTimer {
public:
    explicit PerformanceTimer(const std::string& name, LogLevel level = LogLevel::DEBUG);
    ~PerformanceTimer();
    
    /**
     * @brief Get elapsed time in milliseconds
     */
    double getElapsedMs() const;
    
private:
    std::string name_;
    LogLevel level_;
    std::chrono::high_resolution_clock::time_point start_time_;
};

}} // namespace urology::utils

// Convenient logging macros
#define LOG_TRACE(msg) \
    do { \
        if (urology::utils::Logger::getInstance().isLevelEnabled(urology::utils::LogLevel::TRACE)) { \
            std::ostringstream oss; oss << msg; \
            urology::utils::Logger::getInstance().log(urology::utils::LogLevel::TRACE, oss.str(), __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)

#define LOG_DEBUG(msg) \
    do { \
        if (urology::utils::Logger::getInstance().isLevelEnabled(urology::utils::LogLevel::DEBUG)) { \
            std::ostringstream oss; oss << msg; \
            urology::utils::Logger::getInstance().log(urology::utils::LogLevel::DEBUG, oss.str(), __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)

#define LOG_INFO(msg) \
    do { \
        if (urology::utils::Logger::getInstance().isLevelEnabled(urology::utils::LogLevel::INFO)) { \
            std::ostringstream oss; oss << msg; \
            urology::utils::Logger::getInstance().log(urology::utils::LogLevel::INFO, oss.str(), __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)

#define LOG_WARN(msg) \
    do { \
        if (urology::utils::Logger::getInstance().isLevelEnabled(urology::utils::LogLevel::WARN)) { \
            std::ostringstream oss; oss << msg; \
            urology::utils::Logger::getInstance().log(urology::utils::LogLevel::WARN, oss.str(), __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)

#define LOG_ERROR(msg) \
    do { \
        if (urology::utils::Logger::getInstance().isLevelEnabled(urology::utils::LogLevel::ERROR)) { \
            std::ostringstream oss; oss << msg; \
            urology::utils::Logger::getInstance().log(urology::utils::LogLevel::ERROR, oss.str(), __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)

#define LOG_FATAL(msg) \
    do { \
        std::ostringstream oss; oss << msg; \
        urology::utils::Logger::getInstance().log(urology::utils::LogLevel::FATAL, oss.str(), __FILE__, __LINE__, __FUNCTION__); \
    } while(0)

// Performance timing macros
#define PERF_TIMER(name) urology::utils::PerformanceTimer timer(name)
#define PERF_TIMER_LEVEL(name, level) urology::utils::PerformanceTimer timer(name, level) 