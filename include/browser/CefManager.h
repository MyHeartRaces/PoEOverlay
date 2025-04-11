#pragma once

#include <Windows.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <include/cef_app.h>
#include <include/cef_client.h>
#include "core/Application.h"

namespace poe {

// Forward declarations
class BrowserHandler;
class BrowserClient;
class RenderHandler;

/**
 * @class CefManager
 * @brief Manages the Chromium Embedded Framework lifecycle and browser instances.
 * 
 * This class is responsible for initializing and shutting down CEF,
 * creating browser instances, and managing their lifecycle.
 */
class CefManager {
public:
    /**
     * @struct CefConfig
     * @brief Configuration parameters for CEF.
     */
    struct CefConfig {
        std::string cachePath;           ///< Path to the cache directory
        std::string userDataPath;        ///< Path to the user data directory
        std::string resourcesPath;       ///< Path to the resources directory
        std::string localesPath;         ///< Path to the locales directory
        bool persistSessionCookies = true; ///< Whether to persist session cookies
        bool persistUserPreferences = true; ///< Whether to persist user preferences
        bool enableOffscreenRendering = true; ///< Whether to enable offscreen rendering
        int backgroundProcessPriority = 0; ///< Priority for background processes
        std::string logFile;             ///< Path to the log file
        int logSeverity = 0;             ///< Log severity (0=default, 1=verbose, 2=info, 3=warning, 4=error, 5=fatal)
    };

    /**
     * @brief Constructor for the CefManager class.
     * @param app Reference to the main application instance.
     * @param config Configuration parameters for CEF.
     */
    CefManager(Application& app, const CefConfig& config = CefConfig());

    /**
     * @brief Destructor for the CefManager class.
     */
    ~CefManager();

    // Non-copyable
    CefManager(const CefManager&) = delete;
    CefManager& operator=(const CefManager&) = delete;

    /**
     * @brief Initializes CEF.
     * @return True if initialization was successful, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Shuts down CEF.
     */
    void Shutdown();

    /**
     * @brief Creates a new browser instance.
     * @param url The URL to navigate to.
     * @param width The width of the browser viewport.
     * @param height The height of the browser viewport.
     * @param parent The parent window handle.
     * @param offscreen Whether to create an offscreen browser.
     * @return Handle to the browser instance, or nullptr if creation failed.
     */
    CefRefPtr<CefBrowser> CreateBrowser(
        const std::string& url,
        int width,
        int height,
        HWND parent = nullptr,
        bool offscreen = true
    );

    /**
     * @brief Closes a browser instance.
     * @param browser The browser to close.
     * @param force Whether to force close the browser.
     */
    void CloseBrowser(CefRefPtr<CefBrowser> browser, bool force = false);

    /**
     * @brief Processes CEF events.
     * @param blocking Whether to block until there are no pending events.
     */
    void ProcessEvents(bool blocking = false);

    /**
     * @brief Gets the browser handler.
     * @return Reference to the browser handler.
     */
    BrowserHandler* GetBrowserHandler() const { return m_browserHandler.get(); }

    /**
     * @brief Gets the browser client.
     * @return Reference to the browser client.
     */
    BrowserClient* GetBrowserClient() const { return m_browserClient.get(); }

    /**
     * @brief Gets the main render handler.
     * @return Reference to the render handler.
     */
    RenderHandler* GetRenderHandler() const { return m_renderHandler.get(); }

    /**
     * @brief Gets the CEF configuration.
     * @return Reference to the CEF configuration.
     */
    const CefConfig& GetConfig() const { return m_config; }

private:
    /**
     * @brief Initializes CEF command line arguments.
     * @param args Command line arguments to initialize.
     */
    void InitCommandLineArgs(CefRefPtr<CefCommandLine> args);

    /**
     * @brief Log a message using the application logger.
     * @tparam Args Variadic template for format arguments.
     * @param level The log level (0=trace, 1=debug, 2=info, 3=warning, 4=error, 5=critical).
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    void Log(int level, const std::string& fmt, const Args&... args);

    Application& m_app;                             ///< Reference to the main application
    CefConfig m_config;                             ///< CEF configuration parameters
    bool m_initialized;                             ///< Whether CEF has been initialized
    std::atomic<bool> m_running;                    ///< Whether CEF is running
    
    std::unique_ptr<BrowserHandler> m_browserHandler; ///< Handler for browser events
    std::unique_ptr<BrowserClient> m_browserClient;   ///< CEF client implementation
    std::unique_ptr<RenderHandler> m_renderHandler;   ///< Handler for rendering
    
    mutable std::mutex m_browsersMutex;             ///< Mutex for thread-safe access to browsers
    std::vector<CefRefPtr<CefBrowser>> m_browsers;  ///< List of active browsers
};

} // namespace poe