#include "browser/CefManager.h"
#include "browser/BrowserHandler.h"
#include "browser/BrowserClient.h"
#include "browser/RenderHandler.h"
#include "browser/CefApp.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"
#include "core/Settings.h"

#include <filesystem>
#include <iostream>

namespace poe {

CefManager::CefManager(Application& app, const CefConfig& config)
    : m_app(app)
    , m_config(config)
    , m_initialized(false)
    , m_running(false)
{
    Log(2, "CefManager created");
}

CefManager::~CefManager()
{
    Shutdown();
}

bool CefManager::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    try
    {
        Log(2, "Initializing CefManager");

        // Check if required paths exist
        if (m_config.resourcesPath.empty() || !std::filesystem::exists(m_config.resourcesPath))
        {
            Log(4, "CEF resources path not found: {}", m_config.resourcesPath);
            return false;
        }

        // Create directories if they don't exist
        if (!m_config.cachePath.empty() && !std::filesystem::exists(m_config.cachePath))
        {
            std::filesystem::create_directories(m_config.cachePath);
        }

        if (!m_config.userDataPath.empty() && !std::filesystem::exists(m_config.userDataPath))
        {
            std::filesystem::create_directories(m_config.userDataPath);
        }

        // Initialize CEF
        CefMainArgs mainArgs(GetModuleHandle(nullptr));
        
        // Create CefApp
        CefRefPtr<CefApp> cefApp = new CefApp(*this);
        
        // Initialize with CEF main settings
        CefSettings settings;
        
        // Set subprocess path
        CefString(&settings.browser_subprocess_path).FromString(
            (std::filesystem::path(m_config.resourcesPath) / "CefSubProcess.exe").string());
        
        // Set framework directory path
        CefString(&settings.framework_dir_path).FromString(m_config.resourcesPath);
        
        // Set resources directory path
        CefString(&settings.resources_dir_path).FromString(m_config.resourcesPath);
        
        // Set locales directory path
        CefString(&settings.locales_dir_path).FromString(
            m_config.localesPath.empty() ? 
            (std::filesystem::path(m_config.resourcesPath) / "locales").string() : 
            m_config.localesPath);
        
        // Set cache path
        if (!m_config.cachePath.empty())
        {
            CefString(&settings.cache_path).FromString(m_config.cachePath);
        }
        
        // Set user data path
        if (!m_config.userDataPath.empty())
        {
            CefString(&settings.user_data_path).FromString(m_config.userDataPath);
        }
        
        // Set log file
        if (!m_config.logFile.empty())
        {
            CefString(&settings.log_file).FromString(m_config.logFile);
        }
        
        // Set log severity
        settings.log_severity = static_cast<cef_log_severity_t>(m_config.logSeverity);
        
        // Set background process priority
        settings.background_color = m_config.backgroundProcessPriority;
        
        // Enable offscreen rendering
        settings.windowless_rendering_enabled = m_config.enableOffscreenRendering ? 1 : 0;
        
        // Enable persistent cookies
        settings.persist_session_cookies = m_config.persistSessionCookies ? 1 : 0;
        
        // Enable persistent preferences
        settings.persist_user_preferences = m_config.persistUserPreferences ? 1 : 0;
        
        // Settings required for multi-process mode
        settings.multi_threaded_message_loop = false;
        settings.external_message_pump = false;
        
        // Initialize CEF
        Log(2, "Initializing CEF with process type: main");
        CefInitialize(mainArgs, settings, cefApp, nullptr);
        
        // Create browser handler
        m_browserHandler = std::make_unique<BrowserHandler>(m_app, *this);
        
        // Create render handler
        m_renderHandler = std::make_unique<RenderHandler>(m_app, *this);
        
        // Create browser client
        m_browserClient = std::make_unique<BrowserClient>(
            m_app, 
            *this,
            m_browserHandler.get(),
            m_renderHandler.get()
        );
        
        m_initialized = true;
        m_running = true;
        
        Log(2, "CefManager initialized successfully");
        return true;
    }
    catch (const std::exception& ex)
    {
        m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "CefManager");
        return false;
    }
}

void CefManager::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    Log(2, "Shutting down CefManager");

    // Mark as not running
    m_running = false;

    // Close all browsers
    {
        std::lock_guard<std::mutex> lock(m_browsersMutex);
        for (auto browser : m_browsers)
        {
            if (browser)
            {
                browser->GetHost()->CloseBrowser(true);
            }
        }
        m_browsers.clear();
    }

    // Release handlers
    m_browserHandler.reset();
    m_renderHandler.reset();
    m_browserClient.reset();

    // Shut down CEF
    CefShutdown();

    m_initialized = false;
    Log(2, "CefManager shutdown complete");
}

CefRefPtr<CefBrowser> CefManager::CreateBrowser(
    const std::string& url,
    int width,
    int height,
    HWND parent,
    bool offscreen)
{
    if (!m_initialized)
    {
        Log(4, "Cannot create browser: CefManager not initialized");
        return nullptr;
    }

    try
    {
        Log(2, "Creating browser with URL: {}", url);

        // Create browser settings
        CefBrowserSettings browserSettings;
        browserSettings.windowless_frame_rate = 60; // 60 fps
        
        // Create window info
        CefWindowInfo windowInfo;
        
        if (offscreen)
        {
            // Set up for offscreen rendering
            windowInfo.SetAsWindowless(parent);
        }
        else if (parent)
        {
            // Set up for windowed rendering
            RECT rect = {0, 0, width, height};
            windowInfo.SetAsChild(parent, rect);
        }
        else
        {
            // Set up for popup window
            windowInfo.SetAsPopup(nullptr, "PoEOverlay Browser");
            windowInfo.width = width;
            windowInfo.height = height;
        }
        
        // Create browser
        CefRefPtr<CefBrowser> browser = CefBrowserHost::CreateBrowserSync(
            windowInfo,
            m_browserClient,
            url,
            browserSettings,
            nullptr,
            nullptr
        );
        
        if (browser)
        {
            // Store the browser
            std::lock_guard<std::mutex> lock(m_browsersMutex);
            m_browsers.push_back(browser);
            
            // Set initial viewport size for offscreen browsers
            if (offscreen && m_renderHandler)
            {
                m_renderHandler->Resize(browser, width, height);
            }
            
            Log(2, "Browser created successfully with ID: {}", browser->GetIdentifier());
        }
        else
        {
            Log(4, "Failed to create browser");
        }
        
        return browser;
    }
    catch (const std::exception& ex)
    {
        m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "CefManager");
        return nullptr;
    }
}

void CefManager::CloseBrowser(CefRefPtr<CefBrowser> browser, bool force)
{
    if (!m_initialized || !browser)
    {
        return;
    }

    Log(2, "Closing browser with ID: {}", browser->GetIdentifier());

    // Close the browser
    browser->GetHost()->CloseBrowser(force);
}

void CefManager::ProcessEvents(bool blocking)
{
    if (!m_initialized || !m_running)
    {
        return;
    }

    // Process CEF events
    if (blocking)
    {
        CefDoMessageLoopWork();
    }
    else
    {
        CefRunMessageLoop();
    }
}

void CefManager::InitCommandLineArgs(CefRefPtr<CefCommandLine> args)
{
    if (!args)
    {
        return;
    }

    // Disable sandbox for simplicity
    args->AppendSwitch("no-sandbox");
    
    // Disable GPU acceleration to reduce resource usage
    args->AppendSwitch("disable-gpu");
    args->AppendSwitch("disable-gpu-compositing");
    
    // Disable unnecessary features
    args->AppendSwitch("disable-extensions");
    args->AppendSwitch("disable-pinch");
    
    // Set process type for proper subprocess handling
    if (!args->HasSwitch("type"))
    {
        // This is the browser process
        Log(1, "Setting up command line for browser process");
    }
    else
    {
        // This is a CEF child process
        std::string processType = args->GetSwitchValue("type").ToString();
        Log(1, "Setting up command line for process type: {}", processType);
    }
}

template<typename... Args>
void CefManager::Log(int level, const std::string& fmt, const Args&... args)
{
    try
    {
        auto& logger = m_app.GetLogger();
        
        switch (level)
        {
            case 0: logger.Trace(fmt, args...); break;
            case 1: logger.Debug(fmt, args...); break;
            case 2: logger.Info(fmt, args...); break;
            case 3: logger.Warning(fmt, args...); break;
            case 4: logger.Error(fmt, args...); break;
            case 5: logger.Critical(fmt, args...); break;
            default: logger.Info(fmt, args...); break;
        }
    }
    catch (const std::exception& e)
    {
        // Fallback to stderr if logger is unavailable
        std::cerr << "CefManager log error: " << e.what() << std::endl;
    }
}

} // namespace poe