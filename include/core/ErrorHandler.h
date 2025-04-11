#pragma once

#include <functional>
#include <string>
#include <vector>
#include <mutex>
#include <exception>

namespace poe {

// Forward declarations
class Application;

/**
 * @enum ErrorSeverity
 * @brief Enumeration of error severity levels.
 */
enum class ErrorSeverity {
    Info,       ///< Informational message, not an error
    Warning,    ///< Warning, operation can continue
    Error,      ///< Error, operation may be affected
    Critical,   ///< Critical error, application may be unstable
    Fatal       ///< Fatal error, application cannot continue
};

/**
 * @struct ErrorInfo
 * @brief Structure containing information about an error.
 */
struct ErrorInfo {
    ErrorSeverity severity;    ///< Severity level of the error
    std::string message;       ///< Error message
    std::string component;     ///< Component that generated the error
    std::string details;       ///< Additional error details
    std::exception_ptr exception; ///< Exception pointer if available
};

/**
 * @class ErrorHandler
 * @brief Manages error handling and reporting for the application.
 * 
 * This class provides centralized error handling, allowing components to
 * report errors and register handlers for different types of errors.
 */
class ErrorHandler {
public:
    /**
     * @brief Type definition for error callback functions.
     */
    using ErrorCallback = std::function<void(const ErrorInfo&)>;

    /**
     * @brief Constructor for the ErrorHandler class.
     * @param app Reference to the main application instance.
     */
    explicit ErrorHandler(Application& app);

    /**
     * @brief Destructor for the ErrorHandler class.
     */
    ~ErrorHandler();

    /**
     * @brief Initializes the error handler.
     * @return True if initialization was successful, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Shuts down the error handler.
     */
    void Shutdown();

    /**
     * @brief Reports an error.
     * @param severity Severity level of the error.
     * @param message Error message.
     * @param component Component that generated the error.
     * @param details Additional error details.
     * @param ex Exception pointer if available.
     */
    void ReportError(
        ErrorSeverity severity,
        const std::string& message,
        const std::string& component = "",
        const std::string& details = "",
        std::exception_ptr ex = nullptr
    );

    /**
     * @brief Reports an exception as an error.
     * @param ex The exception to report.
     * @param severity Severity level of the error.
     * @param component Component that generated the error.
     */
    void ReportException(
        const std::exception& ex,
        ErrorSeverity severity = ErrorSeverity::Error,
        const std::string& component = ""
    );

    /**
     * @brief Registers a callback for error notifications.
     * @param callback The callback function to register.
     * @return ID of the registered callback for later removal.
     */
    size_t RegisterErrorCallback(const ErrorCallback& callback);

    /**
     * @brief Unregisters an error callback.
     * @param callbackId ID of the callback to unregister.
     * @return True if callback was found and removed, false otherwise.
     */
    bool UnregisterErrorCallback(size_t callbackId);

    /**
     * @brief Handles an error info object through the registered callbacks.
     * @param errorInfo The error info to handle.
     */
    void HandleError(const ErrorInfo& errorInfo);

    /**
     * @brief Gets the last reported error.
     * @return The last reported error info, or nullptr if no errors have been reported.
     */
    const ErrorInfo* GetLastError() const;

    /**
     * @brief Clears the last error.
     */
    void ClearLastError();

private:
    /**
     * @brief Internal structure for storing callbacks with their IDs.
     */
    struct CallbackEntry {
        size_t id;
        ErrorCallback callback;
    };

    /**
     * @brief Reference to the main application instance.
     */
    Application& m_app;

    /**
     * @brief Vector of registered error callbacks.
     */
    std::vector<CallbackEntry> m_callbacks;

    /**
     * @brief Mutex for thread-safe access to callbacks.
     */
    mutable std::mutex m_callbacksMutex;

    /**
     * @brief The last reported error.
     */
    ErrorInfo m_lastError;

    /**
     * @brief Mutex for thread-safe access to the last error.
     */
    mutable std::mutex m_lastErrorMutex;

    /**
     * @brief Counter for generating unique callback IDs.
     */
    size_t m_nextCallbackId;

    /**
     * @brief Whether fatal error recovery is enabled.
     */
    bool m_fatalRecoveryEnabled;
};

} // namespace poe