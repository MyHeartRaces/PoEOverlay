#include "core/ErrorHandler.h"
#include "core/Application.h"
#include "core/Logger.h"

#include <iostream>

namespace poe {

ErrorHandler::ErrorHandler(Application& app)
    : m_app(app)
    , m_nextCallbackId(1)
    , m_fatalRecoveryEnabled(false)
{
}

ErrorHandler::~ErrorHandler()
{
    Shutdown();
}

bool ErrorHandler::Initialize()
{
    m_app.GetLogger().Info("ErrorHandler initialized");
    
    // Register default error handler that logs errors
    RegisterErrorCallback([this](const ErrorInfo& errorInfo) {
        Logger& logger = m_app.GetLogger();
        
        // Log based on severity
        switch (errorInfo.severity) {
            case ErrorSeverity::Info:
                logger.Info("INFO [{}]: {}", errorInfo.component, errorInfo.message);
                if (!errorInfo.details.empty()) {
                    logger.Info("Details: {}", errorInfo.details);
                }
                break;
                
            case ErrorSeverity::Warning:
                logger.Warning("WARNING [{}]: {}", errorInfo.component, errorInfo.message);
                if (!errorInfo.details.empty()) {
                    logger.Warning("Details: {}", errorInfo.details);
                }
                break;
                
            case ErrorSeverity::Error:
                logger.Error("ERROR [{}]: {}", errorInfo.component, errorInfo.message);
                if (!errorInfo.details.empty()) {
                    logger.Error("Details: {}", errorInfo.details);
                }
                break;
                
            case ErrorSeverity::Critical:
                logger.Critical("CRITICAL [{}]: {}", errorInfo.component, errorInfo.message);
                if (!errorInfo.details.empty()) {
                    logger.Critical("Details: {}", errorInfo.details);
                }
                break;
                
            case ErrorSeverity::Fatal:
                logger.Critical("FATAL [{}]: {}", errorInfo.component, errorInfo.message);
                if (!errorInfo.details.empty()) {
                    logger.Critical("Details: {}", errorInfo.details);
                }
                break;
        }
    });
    
    return true;
}

void ErrorHandler::Shutdown()
{
    // Clear all callbacks
    std::lock_guard<std::mutex> lock(m_callbacksMutex);
    m_callbacks.clear();
    
    m_app.GetLogger().Info("ErrorHandler shutdown");
}

void ErrorHandler::ReportError(
    ErrorSeverity severity,
    const std::string& message,
    const std::string& component,
    const std::string& details,
    std::exception_ptr ex)
{
    ErrorInfo errorInfo;
    errorInfo.severity = severity;
    errorInfo.message = message;
    errorInfo.component = component.empty() ? "Unknown" : component;
    errorInfo.details = details;
    errorInfo.exception = ex;
    
    HandleError(errorInfo);
    
    // Store as last error
    {
        std::lock_guard<std::mutex> lock(m_lastErrorMutex);
        m_lastError = errorInfo;
    }
    
    // If it's a fatal error and recovery is not enabled, exit the application
    if (severity == ErrorSeverity::Fatal && !m_fatalRecoveryEnabled) {
        m_app.GetLogger().Critical("Fatal error, application will exit");
        m_app.Quit(1);
    }
}

void ErrorHandler::ReportException(
    const std::exception& ex,
    ErrorSeverity severity,
    const std::string& component)
{
    ReportError(
        severity,
        ex.what(),
        component,
        "Exception of type: " + std::string(typeid(ex).name()),
        std::current_exception()
    );
}

size_t ErrorHandler::RegisterErrorCallback(const ErrorCallback& callback)
{
    std::lock_guard<std::mutex> lock(m_callbacksMutex);
    
    size_t callbackId = m_nextCallbackId++;
    m_callbacks.push_back({callbackId, callback});
    
    return callbackId;
}

bool ErrorHandler::UnregisterErrorCallback(size_t callbackId)
{
    std::lock_guard<std::mutex> lock(m_callbacksMutex);
    
    for (auto it = m_callbacks.begin(); it != m_callbacks.end(); ++it) {
        if (it->id == callbackId) {
            m_callbacks.erase(it);
            return true;
        }
    }
    
    return false;
}

void ErrorHandler::HandleError(const ErrorInfo& errorInfo)
{
    std::vector<CallbackEntry> callbacks;
    
    // Get a copy of the callbacks to avoid holding the lock during callback execution
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        callbacks = m_callbacks;
    }
    
    // Call each callback
    for (const auto& entry : callbacks) {
        try {
            entry.callback(errorInfo);
        }
        catch (const std::exception& ex) {
            // Use std::cerr instead of the logger to avoid potential infinite recursion
            std::cerr << "Exception in error callback: " << ex.what() << std::endl;
        }
    }
}

const ErrorInfo* ErrorHandler::GetLastError() const
{
    std::lock_guard<std::mutex> lock(m_lastErrorMutex);
    return &m_lastError;
}

void ErrorHandler::ClearLastError()
{
    std::lock_guard<std::mutex> lock(m_lastErrorMutex);
    m_lastError = ErrorInfo();
}

} // namespace poe