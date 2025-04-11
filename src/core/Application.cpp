#include "core/Application.h"
#include "core/Settings.h"
#include "core/Logger.h"
#include "core/EventSystem.h"
#include "core/ErrorHandler.h"

#include <stdexcept>
#include <thread>
#include <chrono>

namespace poe {

// Initialize static instance
Application* Application::s_instance = nullptr;

Application::Application(const std::string& appName)
    : m_appName(appName)
    , m_isRunning(false)
    , m_exitCode(0)
    , m_settings(nullptr)
    , m_logger(nullptr)
    , m_eventSystem(nullptr)
    , m_errorHandler(nullptr)
{
    if (s_instance != nullptr) {
        throw std::runtime_error("Application instance already exists");
    }

    s_instance = this;
}

Application::~Application()
{
    Shutdown();
    s_instance = nullptr;
}

bool Application::Initialize()
{
    try {
        // Create all required subsystems
        if (!CreateSubsystems()) {
            return false;
        }

        // Initialize logger first for proper error logging
        m_logger->Initialize();
        m_logger->Info("Application '{}' initializing...", m_appName);

        // Initialize other subsystems
        m_settings->Initialize();
        m_eventSystem->Initialize();
        m_errorHandler->Initialize();

        m_logger->Info("Application '{}' initialized successfully", m_appName);
        return true;
    }
    catch (const std::exception& e) {
        // If logger is available, log the error
        if (m_logger) {
            m_logger->Error("Failed to initialize application: {}", e.what());
        }
        return false;
    }
}

bool Application::CreateSubsystems()
{
    try {
        // Create subsystems in order of dependency
        m_settings = std::make_unique<Settings>(*this);
        m_logger = std::make_unique<Logger>(*this);
        m_eventSystem = std::make_unique<EventSystem>(*this);
        m_errorHandler = std::make_unique<ErrorHandler>(*this);
        
        return true;
    }
    catch (const std::exception& e) {
        // Cannot use logger here as it might not be initialized
        // In a real application, you might want to output to stderr
        std::cerr << "Failed to create subsystems: " << e.what() << std::endl;
        return false;
    }
}

int Application::Run()
{
    if (!m_isRunning.exchange(true)) {
        m_logger->Info("Application '{}' starting main loop", m_appName);
        
        // Main application loop
        while (m_isRunning.load()) {
            // Process events
            m_eventSystem->ProcessEvents();
            
            // Sleep to avoid high CPU usage in this simple implementation
            // In a real application, this would be replaced with proper event waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        m_logger->Info("Application '{}' main loop ended", m_appName);
    }
    
    return m_exitCode.load();
}

void Application::Shutdown()
{
    if (m_isRunning.exchange(false)) {
        m_logger->Info("Application '{}' shutting down...", m_appName);
        
        // Shutdown subsystems in reverse order of creation
        if (m_errorHandler) m_errorHandler->Shutdown();
        if (m_eventSystem) m_eventSystem->Shutdown();
        if (m_logger) m_logger->Shutdown();
        if (m_settings) m_settings->Shutdown();
        
        // Clear subsystems
        m_errorHandler.reset();
        m_eventSystem.reset();
        m_logger.reset();
        m_settings.reset();
    }
}

void Application::Quit(int exitCode)
{
    m_exitCode.store(exitCode);
    m_isRunning.store(false);
}

Settings& Application::GetSettings() const
{
    if (!m_settings) {
        throw std::runtime_error("Settings subsystem not initialized");
    }
    return *m_settings;
}

Logger& Application::GetLogger() const
{
    if (!m_logger) {
        throw std::runtime_error("Logger subsystem not initialized");
    }
    return *m_logger;
}

EventSystem& Application::GetEventSystem() const
{
    if (!m_eventSystem) {
        throw std::runtime_error("EventSystem subsystem not initialized");
    }
    return *m_eventSystem;
}

ErrorHandler& Application::GetErrorHandler() const
{
    if (!m_errorHandler) {
        throw std::runtime_error("ErrorHandler subsystem not initialized");
    }
    return *m_errorHandler;
}

Application& Application::GetInstance()
{
    if (!s_instance) {
        throw std::runtime_error("Application instance not created yet");
    }
    return *s_instance;
}

} // namespace poe