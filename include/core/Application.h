#pragma once

#include <memory>
#include <string>
#include <atomic>

// Forward declarations
namespace poe {
    class Settings;
    class Logger;
    class EventSystem;
    class ErrorHandler;
}

namespace poe {

/**
 * @class Application
 * @brief Main application class responsible for lifecycle management.
 * 
 * This class is the central component of the application that manages the lifecycle,
 * initializes all subsystems, and handles the main event loop.
 */
class Application {
public:
    /**
     * @brief Constructor for the Application class.
     * @param appName The name of the application.
     */
    explicit Application(const std::string& appName);

    /**
     * @brief Destructor for the Application class.
     */
    ~Application();

    /**
     * @brief Deleted copy constructor.
     */
    Application(const Application&) = delete;

    /**
     * @brief Deleted copy assignment operator.
     */
    Application& operator=(const Application&) = delete;

    /**
     * @brief Deleted move constructor.
     */
    Application(Application&&) = delete;

    /**
     * @brief Deleted move assignment operator.
     */
    Application& operator=(Application&&) = delete;

    /**
     * @brief Initializes the application and all its subsystems.
     * @return True if initialization was successful, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Runs the main application loop.
     * @return Exit code of the application.
     */
    int Run();

    /**
     * @brief Shuts down the application and all its subsystems.
     */
    void Shutdown();

    /**
     * @brief Signals the application to quit.
     * @param exitCode The exit code to return.
     */
    void Quit(int exitCode = 0);

    /**
     * @brief Gets the application name.
     * @return The application name.
     */
    const std::string& GetAppName() const { return m_appName; }

    /**
     * @brief Gets the settings manager.
     * @return Reference to the settings manager.
     */
    Settings& GetSettings() const;

    /**
     * @brief Gets the logger.
     * @return Reference to the logger.
     */
    Logger& GetLogger() const;

    /**
     * @brief Gets the event system.
     * @return Reference to the event system.
     */
    EventSystem& GetEventSystem() const;

    /**
     * @brief Gets the error handler.
     * @return Reference to the error handler.
     */
    ErrorHandler& GetErrorHandler() const;

    /**
     * @brief Gets the instance of the application.
     * @return Reference to the singleton instance.
     * @throws std::runtime_error if instance is not created yet.
     */
    static Application& GetInstance();

private:
    /**
     * @brief Creates all required subsystems.
     * @return True if creation was successful, false otherwise.
     */
    bool CreateSubsystems();

    /**
     * @brief Static instance of the application for singleton pattern.
     */
    static Application* s_instance;

    /**
     * @brief Name of the application.
     */
    std::string m_appName;

    /**
     * @brief Flag indicating if the application is running.
     */
    std::atomic<bool> m_isRunning;

    /**
     * @brief Exit code to return from the application.
     */
    std::atomic<int> m_exitCode;

    /**
     * @brief Settings manager subsystem.
     */
    std::unique_ptr<Settings> m_settings;

    /**
     * @brief Logger subsystem.
     */
    std::unique_ptr<Logger> m_logger;

    /**
     * @brief Event system subsystem.
     */
    std::unique_ptr<EventSystem> m_eventSystem;

    /**
     * @brief Error handler subsystem.
     */
    std::unique_ptr<ErrorHandler> m_errorHandler;
};

} // namespace poe