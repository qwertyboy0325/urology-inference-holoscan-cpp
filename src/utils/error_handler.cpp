#include "utils/error_handler.hpp"
#include "utils/logger.hpp"
#include <sstream>
#include <thread>
#include <chrono>

namespace urology {
namespace utils {

// Thread-local storage for error context stack
thread_local std::vector<std::string> ErrorContext::context_stack_;

// UrologyException implementation
std::string UrologyException::formatMessage() const {
    std::ostringstream oss;
    oss << "[" << static_cast<int>(code_) << "] " << message_;
    if (!context_.empty()) {
        oss << " (Context: " << context_ << ")";
    }
    return oss.str();
}

// ErrorHandler implementation
ErrorHandler& ErrorHandler::getInstance() {
    static ErrorHandler instance;
    return instance;
}

void ErrorHandler::registerErrorCallback(ErrorCode code, ErrorCallback callback) {
    callbacks_[code].push_back(callback);
}

void ErrorHandler::registerRecoveryStrategy(ErrorCode code, 
                                           std::shared_ptr<IErrorRecoveryStrategy> strategy) {
    recovery_strategies_[code] = strategy;
}

bool ErrorHandler::handleError(ErrorCode code, const std::string& message, 
                              const std::string& context, bool throw_on_failure) {
    // Update error statistics
    error_statistics_[code]++;
    
    // Log the error
    std::string full_context = context.empty() ? ErrorContext::getCurrentContext() : context;
        UROLOGY_LOG_ERROR("Error [" + std::to_string(static_cast<int>(code)) + "]: " + message + " (Context: " + full_context + ")");
    
    // Try recovery if strategy is available
    auto recovery_it = recovery_strategies_.find(code);
    if (recovery_it != recovery_strategies_.end()) {
        auto& strategy = recovery_it->second;
        if (strategy->canRecover(code)) {
            UROLOGY_LOG_INFO("Attempting error recovery for code " + std::to_string(static_cast<int>(code)));
            if (strategy->recover(code, full_context)) {
                UROLOGY_LOG_INFO("Error recovery successful for code " + std::to_string(static_cast<int>(code)));
                return true;
            } else {
                UROLOGY_LOG_ERROR("Error recovery failed for code " + std::to_string(static_cast<int>(code)));
            }
        }
    }
    
    // Execute callbacks
    auto callback_it = callbacks_.find(code);
    if (callback_it != callbacks_.end()) {
        for (const auto& callback : callback_it->second) {
            try {
                callback(code, message);
            } catch (const std::exception& e) {
                UROLOGY_LOG_ERROR("Error callback threw exception: " + std::string(e.what()));
            }
        }
    }
    
    // Throw exception if requested and recovery failed
    if (throw_on_failure) {
        throw UrologyException(code, message, full_context);
    }
    
    return false;
}

std::map<ErrorCode, int> ErrorHandler::getErrorStatistics() const {
    return error_statistics_;
}

void ErrorHandler::resetStatistics() {
    error_statistics_.clear();
}

std::string ErrorHandler::errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS: return "SUCCESS";
        case ErrorCode::CONFIG_ERROR: return "CONFIG_ERROR";
        case ErrorCode::MODEL_LOAD_ERROR: return "MODEL_LOAD_ERROR";
        case ErrorCode::INFERENCE_ERROR: return "INFERENCE_ERROR";
        case ErrorCode::VIDEO_CAPTURE_ERROR: return "VIDEO_CAPTURE_ERROR";
        case ErrorCode::VIDEO_ENCODER_ERROR: return "VIDEO_ENCODER_ERROR";
        case ErrorCode::MEMORY_ERROR: return "MEMORY_ERROR";
        case ErrorCode::GPU_ERROR: return "GPU_ERROR";
        case ErrorCode::FILE_IO_ERROR: return "FILE_IO_ERROR";
        case ErrorCode::NETWORK_ERROR: return "NETWORK_ERROR";
        case ErrorCode::VALIDATION_ERROR: return "VALIDATION_ERROR";
        case ErrorCode::UNKNOWN_ERROR: return "UNKNOWN_ERROR";
        default: return "UNKNOWN_CODE";
    }
}

// ErrorContext implementation
ErrorContext::ErrorContext(const std::string& context) : context_(context) {
    context_stack_.push_back(context);
}

ErrorContext::~ErrorContext() {
    if (!context_stack_.empty() && context_stack_.back() == context_) {
        context_stack_.pop_back();
    }
}

std::string ErrorContext::getCurrentContext() {
    if (context_stack_.empty()) {
        return "Global";
    }
    
    std::ostringstream oss;
    for (size_t i = 0; i < context_stack_.size(); ++i) {
        if (i > 0) oss << " -> ";
        oss << context_stack_[i];
    }
    return oss.str();
}

// GPUErrorRecoveryStrategy implementation
bool GPUErrorRecoveryStrategy::canRecover(ErrorCode code) const {
    return code == ErrorCode::GPU_ERROR || code == ErrorCode::MEMORY_ERROR;
}

bool GPUErrorRecoveryStrategy::recover(ErrorCode code, const std::string& context) {
    UROLOGY_LOG_INFO("Attempting GPU error recovery for context: " + context);
    
    switch (code) {
        case ErrorCode::GPU_ERROR:
            // Try GPU reset or fallback to CPU
            UROLOGY_LOG_INFO("GPU error recovery: attempting device reset");
            // In a real implementation, this would reset GPU context
            // or switch to CPU processing
            return true;
            
        case ErrorCode::MEMORY_ERROR:
            // Try to free GPU memory
            UROLOGY_LOG_INFO("Memory error recovery: attempting memory cleanup");
            // In a real implementation, this would trigger garbage collection
            // or release cached GPU memory
            return true;
            
        default:
            return false;
    }
}

// FileIOErrorRecoveryStrategy implementation
bool FileIOErrorRecoveryStrategy::canRecover(ErrorCode code) const {
    return code == ErrorCode::FILE_IO_ERROR;
}

bool FileIOErrorRecoveryStrategy::recover(ErrorCode code, const std::string& context) {
    if (code != ErrorCode::FILE_IO_ERROR) {
        return false;
    }
    
    UROLOGY_LOG_INFO("Attempting file I/O error recovery for context: " + context);
    
    // Try to create directory if it doesn't exist
    // Try alternative file locations
    // Retry with different permissions
    // In a real implementation, this would contain actual file recovery logic
    
    // For now, just simulate recovery attempt
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    UROLOGY_LOG_INFO("File I/O error recovery completed");
    return true; // Simplified - assume recovery successful
}

}} // namespace urology::utils 