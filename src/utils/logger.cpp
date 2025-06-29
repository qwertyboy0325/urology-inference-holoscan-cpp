#include "utils/logger.hpp"
#include <iostream>
#include <filesystem>

namespace urology {
namespace utils {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const std::string& log_file, LogLevel level, bool console_output) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    min_level_ = level;
    console_output_ = console_output;
    
    if (!log_file.empty()) {
        // Create log directory if it doesn't exist
        std::filesystem::path log_path(log_file);
        if (log_path.has_parent_path()) {
            std::filesystem::create_directories(log_path.parent_path());
        }
        
        file_stream_.open(log_file, std::ios::out | std::ios::app);
        if (file_stream_.is_open()) {
            file_output_ = true;
            log(LogLevel::INFO, "Logger initialized with file output: " + log_file);
        } else {
            std::cerr << "Failed to open log file: " << log_file << std::endl;
            file_output_ = false;
        }
    }
    
    initialized_ = true;
}

Logger::~Logger() {
    flush();
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

void Logger::log(LogLevel level, const std::string& message, 
                const char* file, int line, const char* func) {
    if (level < min_level_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string formatted_msg = formatMessage(level, message, file, line, func);
    
    // Console output
    if (console_output_) {
        if (level >= LogLevel::ERROR) {
            std::cerr << formatted_msg << std::endl;
        } else {
            std::cout << formatted_msg << std::endl;
        }
    }
    
    // File output
    if (file_output_ && file_stream_.is_open()) {
        file_stream_ << formatted_msg << std::endl;
        if (level >= LogLevel::ERROR) {
            file_stream_.flush(); // Immediate flush for errors
        }
    }
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (console_output_) {
        std::cout.flush();
        std::cerr.flush();
    }
    if (file_output_ && file_stream_.is_open()) {
        file_stream_.flush();
    }
}

std::string Logger::formatMessage(LogLevel level, const std::string& message,
                                 const char* file, int line, const char* func) {
    std::ostringstream oss;
    
    // Timestamp
    oss << "[" << getCurrentTime() << "] ";
    
    // Log level
    oss << "[" << getLogLevelString(level) << "] ";
    
    // File location (only filename, not full path)
    if (file) {
        std::string filename = std::filesystem::path(file).filename().string();
        oss << "[" << filename << ":" << line << "] ";
    }
    
    // Function name
    if (func) {
        oss << "[" << func << "] ";
    }
    
    // Message
    oss << message;
    
    return oss.str();
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

std::string Logger::getLogLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

// PerformanceTimer implementation
PerformanceTimer::PerformanceTimer(const std::string& name, LogLevel level)
    : name_(name), level_(level), start_time_(std::chrono::high_resolution_clock::now()) {
    if (Logger::getInstance().isLevelEnabled(level_)) {
        Logger::getInstance().log(level_, "Performance timer started: " + name_);
    }
}

PerformanceTimer::~PerformanceTimer() {
    if (Logger::getInstance().isLevelEnabled(level_)) {
        double elapsed_ms = getElapsedMs();
        std::ostringstream oss;
        oss << "Performance timer finished: " << name_ 
            << " (elapsed: " << std::fixed << std::setprecision(3) << elapsed_ms << " ms)";
        Logger::getInstance().log(level_, oss.str());
    }
}

double PerformanceTimer::getElapsedMs() const {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
    return duration.count() / 1000.0;
}

}} // namespace urology::utils 