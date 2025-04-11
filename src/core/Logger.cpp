#include "core/Logger.h"
#include "core/Application.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/fmt.h>

#include <vector>
#include <iostream>

namespace poe {

Logger::Logger(Application& app)
    : m_app(app)
    , m_logger(nullptr)
    , m_consoleLoggingEnabled(true)
{
    // Set default log file path
    auto appDataPath = std::filesystem::temp_directory_path() / "PoEOverlay";
    m_logFilePath = appDataPath / "logs" / "poeoverlay.log";
}

Logger::~Logger()
{
    Shutdown();
}

bool Logger::Initialize()
{
    try {
        // Create directories if they don't exist
        std::filesystem::create_directories(m_logFilePath.parent_path());
        
        // Create loggers
        CreateLoggers();
        
        Info("Logger initialized");
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
        return false;
    }
}

void Logger::Shutdown()
{
    if (m_logger) {
        Info("Logger shutting down");
        // This will flush and close all sinks
        spdlog::drop_all();
        m_logger = nullptr;
    }
}

void Logger::SetLevel(int level)
{
    if (m_logger) {
        spdlog::level::level_enum spdlogLevel = spdlog::level::info;
        
        switch (level) {
            case 0: spdlogLevel = spdlog::level::trace; break;
            case 1: spdlogLevel = spdlog::level::debug; break;
            case 2: spdlogLevel = spdlog::level::info; break;
            case 3: spdlogLevel = spdlog::level::warn; break;
            case 4: spdlogLevel = spdlog::level::err; break;
            case 5: spdlogLevel = spdlog::level::critical; break;
            case 6: spdlogLevel = spdlog::level::off; break;
            default: spdlogLevel = spdlog::level::info; break;
        }
        
        m_logger->set_level(spdlogLevel);
    }
}

void Logger::SetLogFilePath(const std::filesystem::path& path)
{
    if (m_logFilePath != path) {
        m_logFilePath = path;
        
        // Reinitialize loggers with new path
        if (m_logger) {
            CreateLoggers();
        }
    }
}

const std::filesystem::path& Logger::GetLogFilePath() const
{
    return m_logFilePath;
}

void Logger::EnableConsoleLogging(bool enable)
{
    if (m_consoleLoggingEnabled != enable) {
        m_consoleLoggingEnabled = enable;
        
        // Reinitialize loggers with new settings
        if (m_logger) {
            CreateLoggers();
        }
    }
}

template<typename... Args>
void Logger::Log(int level, std::string_view fmt, const Args&... args)
{
    if (!m_logger) return;
    
    try {
        switch (level) {
            case 0: m_logger->trace(fmt, args...); break;
            case 1: m_logger->debug(fmt, args...); break;
            case 2: m_logger->info(fmt, args...); break;
            case 3: m_logger->warn(fmt, args...); break;
            case 4: m_logger->error(fmt, args...); break;
            case 5: m_logger->critical(fmt, args...); break;
            default: m_logger->info(fmt, args...); break;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Logging error: " << e.what() << std::endl;
    }
}

void Logger::CreateLoggers()
{
    // Drop existing logger if any
    if (m_logger) {
        spdlog::drop("poeoverlay");
    }
    
    // Create sinks
    std::vector<spdlog::sink_ptr> sinks;
    
    // File sink (rotating, max 5MB per file, max 3 files)
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        m_logFilePath.string(), 5 * 1024 * 1024, 3);
    sinks.push_back(file_sink);
    
    // Console sink if enabled
    if (m_consoleLoggingEnabled) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sinks.push_back(console_sink);
    }
    
    // Create and register logger
    m_logger = std::make_shared<spdlog::logger>("poeoverlay", sinks.begin(), sinks.end());
    spdlog::register_logger(m_logger);
    
    // Set log format
    m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    
    // Set default level (info)
    m_logger->set_level(spdlog::level::info);
    
    // Set as default logger
    spdlog::set_default_logger(m_logger);
}

// Explicit template instantiations for common types
template void Logger::Log(int, std::string_view);
template void Logger::Log(int, std::string_view, const std::string&);
template void Logger::Log(int, std::string_view, const char*);
template void Logger::Log(int, std::string_view, int);
template void Logger::Log(int, std::string_view, double);
template void Logger::Log(int, std::string_view, bool);

} // namespace poe