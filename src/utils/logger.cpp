#include "utils/logger.hpp"
#include <iostream>
#include <thread>
#include <cstring>

namespace urology {
namespace utils {

UrologyLogger& UrologyLogger::getInstance() {
    static UrologyLogger instance;
    return instance;
}

void UrologyLogger::initialize(const std::string& log_file, 
                              UrologyLogLevel level, 
                              bool console_output) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    min_level_ = level;
    console_output_ = console_output;
    
    if (!log_file.empty()) {
        file_stream_.open(log_file, std::ios::app);
        file_output_ = file_stream_.is_open();
        if (!file_output_) {
            std::cerr << "Failed to open log file: " << log_file << std::endl;
        }
    }
    
    initialized_ = true;
}

void UrologyLogger::log(UrologyLogLevel level, const std::string& message,
                       const char* file, int line, const char* func) {
    if (!isLevelEnabled(level)) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string formatted_message = formatMessage(level, message, file, line, func);
    
    if (console_output_) {
        if (level >= UrologyLogLevel::ERROR) {
            std::cerr << formatted_message << std::endl;
        } else {
            std::cout << formatted_message << std::endl;
        }
    }
    
    if (file_output_ && file_stream_.is_open()) {
        file_stream_ << formatted_message << std::endl;
        file_stream_.flush();
    }
}

void UrologyLogger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_output_ && file_stream_.is_open()) {
        file_stream_.flush();
    }
}

UrologyLogger::~UrologyLogger() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

std::string UrologyLogger::formatMessage(UrologyLogLevel level, const std::string& message,
                                        const char* file, int line, const char* func) {
    std::ostringstream oss;
    
    // Timestamp
    oss << "[" << getCurrentTime() << "] ";
    
    // Log level
    oss << "[" << getLogLevelString(level) << "] ";
    
    // Thread ID
    oss << "[T" << std::this_thread::get_id() << "] ";
    
    // Location info (only filename, not full path)
    const char* filename = strrchr(file, '/');
    filename = filename ? filename + 1 : file;
    oss << "[" << filename << ":" << line << "] ";
    
    // Function name
    oss << "[" << func << "] ";
    
    // Message
    oss << message;
    
    return oss.str();
}

std::string UrologyLogger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

std::string UrologyLogger::getLogLevelString(UrologyLogLevel level) {
    switch (level) {
        case UrologyLogLevel::TRACE: return "TRACE";
        case UrologyLogLevel::DEBUG: return "DEBUG";
        case UrologyLogLevel::INFO:  return "INFO ";
        case UrologyLogLevel::WARN:  return "WARN ";
        case UrologyLogLevel::ERROR: return "ERROR";
        case UrologyLogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

// Performance Timer Implementation
UrologyPerformanceTimer::UrologyPerformanceTimer(const std::string& name, UrologyLogLevel level)
    : name_(name), level_(level), start_time_(std::chrono::high_resolution_clock::now()) {
}

UrologyPerformanceTimer::~UrologyPerformanceTimer() {
    double elapsed_ms = getElapsedMs();
    
    std::ostringstream oss;
    oss << "Performance [" << name_ << "]: " << elapsed_ms << " ms";
    
    UrologyLogger::getInstance().log(level_, oss.str(), __FILE__, __LINE__, __FUNCTION__);
}

double UrologyPerformanceTimer::getElapsedMs() const {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
    return duration.count() / 1000.0;
}

}} // namespace urology::utils 