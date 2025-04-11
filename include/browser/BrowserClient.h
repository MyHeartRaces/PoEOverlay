#pragma once

#include <include/cef_client.h>
#include "core/Application.h"

namespace poe {

// Forward declarations
class Application;
class CefManager;
class BrowserHandler;
class RenderHandler;

/**
 * @class BrowserClient
 * @brief Implements the CEF client interface.
 * 
 * This class serves as the main integration point between the
 * application and CEF, providing access to the various handlers.
 */
class BrowserClient : public CefClient {
public:
    /**
     * @brief Constructor for the BrowserClient class.
     * @param app Reference to the main application instance.
     * @param cefManager Reference to the CEF manager.
     * @param browserHandler Pointer to the browser handler.
     * @param renderHandler Pointer to the render handler.
     */
    BrowserClient(
        Application& app,
        CefManager& cefManager,
        BrowserHandler* browserHandler,
        RenderHandler* renderHandler
    );

    // CefClient methods
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override;
    CefRefPtr<CefLoadHandler> GetLoadHandler() override;
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override;
    CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override;
    CefRefPtr<CefRenderHandler> GetRenderHandler() override;
    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame, CefProcessId source_process,
        CefRefPtr<CefProcessMessage> message) override;

private:
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
    IMPLEMENT_REFCOUNTING(BrowserClient);

    Application& m_app;                       ///< Reference to the main application
    CefManager& m_cefManager;                 ///< Reference to the CEF manager
    BrowserHandler* m_browserHandler;         ///< Pointer to the browser handler
    RenderHandler* m_renderHandler;           ///< Pointer to the render handler
};

} // namespace poe