#pragma once

#include <Windows.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <include/cef_client.h>
#include <include/cef_life_span_handler.h>
#include <include/cef_load_handler.h>
#include <include/cef_display_handler.h>
#include <include/cef_context_menu_handler.h>
#include "core/Application.h"

namespace poe {

// Forward declarations
class Application;
class CefManager;

/**
 * @class BrowserHandler
 * @brief Handles browser-related events and callbacks.
 * 
 * This class implements various CEF handler interfaces to handle
 * browser lifecycle, loading, display, and context menu events.
 */
class BrowserHandler : 
    public CefLifeSpanHandler,
    public CefLoadHandler,
    public CefDisplayHandler,
    public CefContextMenuHandler
{
public:
    /**
     * @brief Type definition for browser creation callback.
     */
    using BrowserCreatedCallback = std::function<void(CefRefPtr<CefBrowser>)>;

    /**
     * @brief Type definition for browser close callback.
     */
    using BrowserCloseCallback = std::function<void(CefRefPtr<CefBrowser>)>;

    /**
     * @brief Type definition for browser load state change callback.
     */
    using LoadStateCallback = std::function<void(CefRefPtr<CefBrowser>, bool isLoading, bool canGoBack, bool canGoForward)>;

    /**
     * @brief Type definition for browser title change callback.
     */
    using TitleChangeCallback = std::function<void(CefRefPtr<CefBrowser>, const std::string& title)>;

    /**
     * @brief Type definition for browser address change callback.
     */
    using AddressChangeCallback = std::function<void(CefRefPtr<CefBrowser>, const std::string& url)>;

    /**
     * @brief Type definition for browser status message callback.
     */
    using StatusMessageCallback = std::function<void(CefRefPtr<CefBrowser>, const std::string& message)>;

    /**
     * @brief Constructor for the BrowserHandler class.
     * @param app Reference to the main application instance.
     * @param cefManager Reference to the CEF manager.
     */
    BrowserHandler(Application& app, CefManager& cefManager);

    /**
     * @brief Sets the callback for browser creation events.
     * @param callback The callback function.
     */
    void SetBrowserCreatedCallback(BrowserCreatedCallback callback) { m_browserCreatedCallback = callback; }

    /**
     * @brief Sets the callback for browser close events.
     * @param callback The callback function.
     */
    void SetBrowserCloseCallback(BrowserCloseCallback callback) { m_browserCloseCallback = callback; }

    /**
     * @brief Sets the callback for load state change events.
     * @param callback The callback function.
     */
    void SetLoadStateCallback(LoadStateCallback callback) { m_loadStateCallback = callback; }

    /**
     * @brief Sets the callback for title change events.
     * @param callback The callback function.
     */
    void SetTitleChangeCallback(TitleChangeCallback callback) { m_titleChangeCallback = callback; }

    /**
     * @brief Sets the callback for address change events.
     * @param callback The callback function.
     */
    void SetAddressChangeCallback(AddressChangeCallback callback) { m_addressChangeCallback = callback; }

    /**
     * @brief Sets the callback for status message events.
     * @param callback The callback function.
     */
    void SetStatusMessageCallback(StatusMessageCallback callback) { m_statusMessageCallback = callback; }

    // CefLifeSpanHandler methods
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    bool OnBeforePopup(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
        const CefString& target_url, const CefString& target_frame_name,
        CefLifeSpanHandler::WindowOpenDisposition target_disposition,
        bool user_gesture, const CefPopupFeatures& popupFeatures,
        CefWindowInfo& windowInfo, CefRefPtr<CefClient>& client,
        CefBrowserSettings& settings, CefRefPtr<CefDictionaryValue>& extra_info,
        bool* no_javascript_access) override;
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    bool DoClose(CefRefPtr<CefBrowser> browser) override;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    // CefLoadHandler methods
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading,
        bool canGoBack, bool canGoForward) override;
    void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
        ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl) override;

    // CefDisplayHandler methods
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;
    void OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url) override;
    void OnStatusMessage(CefRefPtr<CefBrowser> browser, const CefString& value) override;
    bool OnConsoleMessage(CefRefPtr<CefBrowser> browser, cef_log_severity_t level,
        const CefString& message, const CefString& source, int line) override;

    // CefContextMenuHandler methods
    CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }
    void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
        CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model) override;
    bool OnContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
        CefRefPtr<CefContextMenuParams> params, int command_id, EventFlags event_flags) override;

private:
    /**
     * @brief Stores browser-specific data.
     */
    struct BrowserData {
        std::string url;
        std::string title;
        bool isLoading = false;
        bool canGoBack = false;
        bool canGoForward = false;
    };

    /**
     * @brief Log a message using the application logger.
     * @tparam Args Variadic template for format arguments.
     * @param level The log level (0=trace, 1=debug, 2=info, 3=warning, 4=error, 5=critical).
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    void Log(int level, const std::string& fmt, const Args&... args);

    // Required for IMPLEMENT_REFCOUNTING
    IMPLEMENT_REFCOUNTING(BrowserHandler);

    Application& m_app;                             ///< Reference to the main application
    CefManager& m_cefManager;                       ///< Reference to the CEF manager
    
    std::unordered_map<int, BrowserData> m_browserData; ///< Map of browser ID to browser data
    std::mutex m_browserDataMutex;                 ///< Mutex for thread-safe access to browser data
    
    BrowserCreatedCallback m_browserCreatedCallback; ///< Callback for browser creation events
    BrowserCloseCallback m_browserCloseCallback;    ///< Callback for browser close events
    LoadStateCallback m_loadStateCallback;          ///< Callback for load state change events
    TitleChangeCallback m_titleChangeCallback;      ///< Callback for title change events
    AddressChangeCallback m_addressChangeCallback;  ///< Callback for address change events
    StatusMessageCallback m_statusMessageCallback;  ///< Callback for status message events
};

} // namespace poe