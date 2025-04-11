#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <filesystem>

// Forward declarations for spdlog classes
namespace spdlog {
    class logger;
}

namespace poe {

// Forward declarations
class Application;

/**
 * @class Logger
 * @brief Provides logging functionality for the application.
 * 
 * This class is a wrapper around spdlog providing formatted logging
 * at various levels with file and console output capabilities.
 */
class Logger {
public:
    /**
     * @brief Constructor for the Logger class.
     * @param app Reference to the main application instance.
     */
    explicit Logger(Application& app);

    /**
     * @brief Destructor for the Logger class.
     */
    ~Logger();

    /**
     * @brief Initializes the logger.
     * @return True if initialization was successful, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Shuts down the logger.
     */
    void Shutdown();

    /**
     * @brief Sets the log level.
     * @param level The log level (0=trace, 1=debug, 2=info, 3=warning, 4=error, 5=critical, 6=off).
     */
    void SetLevel(int level);

    /**
     * @brief Sets the log file path.
     * @param path The path to the log file.
     */
    void SetLogFilePath(const std::filesystem::path& path);

    /**
     * @brief Gets the log file path.
     * @return The path to the log file.
     */
    const std::filesystem::path& GetLogFilePath() const;

    /**
     * @brief Enables or disables console logging.
     * @param enable Whether to enable console logging.
     */
    void EnableConsoleLogging(bool enable);

    /**
     * @brief Logs a message at trace level.
     * @tparam Args Variadic template for format arguments.
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    void Trace(std::string_view fmt, const Args&... args) {
        Log(0, fmt, args...);
    }

    /**
     * @brief Logs a message at debug level.
     * @tparam Args Variadic template for format arguments.
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    void Debug(std::string_view fmt, const Args&... args) {
        Log(1, fmt, args...);
    }

    /**
     * @brief Logs a message at info level.
     * @tparam Args Variadic template for format arguments.
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    void Info(std::string_view fmt, const Args&... args) {
        Log(2, fmt, args...);
    }

    /**
     * @brief Logs a message at warning level.
     * @tparam Args Variadic template for format arguments.
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    void Warning(std::string_view fmt, const Args&... args) {
        Log(3, fmt, args...);
    }

    /**
     * @brief Logs a message at error level.
     * @tparam Args Variadic template for format arguments.
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    void Error(std::string_view fmt, const Args&... args) {
        Log(4, fmt, args...);
    }

    /**
     * @brief Logs a message at critical level.
     * @tparam Args Variadic template for format arguments.
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    void Critical(std::string_view fmt, const Args&... args) {
        Log(5, fmt, args...);
    }

private:
    /**
     * @brief Internal logging function.
     * @tparam Args Variadic template for format arguments.
     * @param level The log level.
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    void Log(int level, std::string_view fmt, const Args&... args);

    /**
     * @brief Creates the logger backends.
     */
    void CreateLoggers();

    /**
     * @brief Reference to the main application instance.
     */
    Application& m_app;

    /**
     * @brief The main logger instance.
     */
    std::shared_ptr<spdlog::logger> m_logger;

    /**
     * @brief Path to the log file.
     */
    std::filesystem::path m_logFilePath;

    /**
     * @brief Whether console logging is enabled.
     */
    bool m_consoleLoggingEnabled;
};

} // namespace poe