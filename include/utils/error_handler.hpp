#pragma once

#include <stdexcept>
#include <string>
#include <functional>
#include <map>
#include <vector>
#include <memory>

namespace urology {
namespace utils {

/**
 * @brief Error codes for different types of application errors
 */
enum class ErrorCode {
    SUCCESS = 0,
    CONFIG_ERROR = 1001,
    MODEL_LOAD_ERROR = 1002,
    INFERENCE_ERROR = 1003,
    VIDEO_CAPTURE_ERROR = 1004,
    VIDEO_ENCODER_ERROR = 1005,
    MEMORY_ERROR = 1006,
    GPU_ERROR = 1007,
    FILE_IO_ERROR = 1008,
    NETWORK_ERROR = 1009,
    VALIDATION_ERROR = 1010,
    UNKNOWN_ERROR = 9999
};

/**
 * @brief Custom exception class with error codes and context
 */
class UrologyException : public std::exception {
public:
    UrologyException(ErrorCode code, const std::string& message, 
                    const std::string& context = "")
        : code_(code), message_(message), context_(context) {
        formatted_message_ = formatMessage();
    }
    
    const char* what() const noexcept override {
        return formatted_message_.c_str();
    }
    
    ErrorCode getErrorCode() const { return code_; }
    const std::string& getMessage() const { return message_; }
    const std::string& getContext() const { return context_; }

private:
    ErrorCode code_;
    std::string message_;
    std::string context_;
    std::string formatted_message_;
    
    std::string formatMessage() const;
};

/**
 * @brief Error recovery strategy interface
 */
class IErrorRecoveryStrategy {
public:
    virtual ~IErrorRecoveryStrategy() = default;
    virtual bool canRecover(ErrorCode code) const = 0;
    virtual bool recover(ErrorCode code, const std::string& context) = 0;
};

/**
 * @brief Centralized error handler with recovery strategies
 */
class ErrorHandler {
public:
    using ErrorCallback = std::function<void(ErrorCode, const std::string&)>;
    
    /**
     * @brief Get the global error handler instance
     */
    static ErrorHandler& getInstance();
    
    /**
     * @brief Register an error callback
     */
    void registerErrorCallback(ErrorCode code, ErrorCallback callback);
    
    /**
     * @brief Register a recovery strategy
     */
    void registerRecoveryStrategy(ErrorCode code, 
                                 std::shared_ptr<IErrorRecoveryStrategy> strategy);
    
    /**
     * @brief Handle an error with potential recovery
     * @param code Error code
     * @param message Error message
     * @param context Additional context
     * @param throw_on_failure Whether to throw exception if recovery fails
     * @return true if error was handled/recovered, false otherwise
     */
    bool handleError(ErrorCode code, const std::string& message, 
                    const std::string& context = "", bool throw_on_failure = true);
    
    /**
     * @brief Get error statistics
     */
    std::map<ErrorCode, int> getErrorStatistics() const;
    
    /**
     * @brief Reset error statistics
     */
    void resetStatistics();
    
    /**
     * @brief Convert error code to string
     */
    static std::string errorCodeToString(ErrorCode code);

private:
    ErrorHandler() = default;
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;
    
    std::map<ErrorCode, std::vector<ErrorCallback>> callbacks_;
    std::map<ErrorCode, std::shared_ptr<IErrorRecoveryStrategy>> recovery_strategies_;
    std::map<ErrorCode, int> error_statistics_;
};

/**
 * @brief RAII error context manager
 */
class ErrorContext {
public:
    explicit ErrorContext(const std::string& context);
    ~ErrorContext();
    
    static std::string getCurrentContext();

private:
    std::string context_;
    static thread_local std::vector<std::string> context_stack_;
};

/**
 * @brief GPU error recovery strategy
 */
class GPUErrorRecoveryStrategy : public IErrorRecoveryStrategy {
public:
    bool canRecover(ErrorCode code) const override;
    bool recover(ErrorCode code, const std::string& context) override;
};

/**
 * @brief File I/O error recovery strategy
 */
class FileIOErrorRecoveryStrategy : public IErrorRecoveryStrategy {
public:
    bool canRecover(ErrorCode code) const override;
    bool recover(ErrorCode code, const std::string& context) override;
};

}} // namespace urology::utils

// Convenient error handling macros
#define HANDLE_ERROR(code, message) \
    urology::utils::ErrorHandler::getInstance().handleError(code, message, \
        urology::utils::ErrorContext::getCurrentContext())

#define HANDLE_ERROR_NO_THROW(code, message) \
    urology::utils::ErrorHandler::getInstance().handleError(code, message, \
        urology::utils::ErrorContext::getCurrentContext(), false)

#define ERROR_CONTEXT(name) urology::utils::ErrorContext error_ctx(name)

#define TRY_RECOVER(code, message, action) \
    do { \
        if (!HANDLE_ERROR_NO_THROW(code, message)) { \
            action; \
        } \
    } while(0) 