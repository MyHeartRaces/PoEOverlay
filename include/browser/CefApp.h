#pragma once

#include <include/cef_app.h>
#include <include/cef_browser_process_handler.h>
#include <include/cef_render_process_handler.h>
#include "core/Application.h"

namespace poe {

// Forward declarations
class CefManager;

/**
 * @class CefApp
 * @brief Implementation of the CEF application interface.
 * 
 * This class serves as the entry point for CEF functionality
 * and handles process-specific operations.
 */
class CefApp : 
    public CefApp,
    public CefBrowserProcessHandler,
    public CefRenderProcessHandler
{
public:
    /**
     * @brief Constructor for the CefApp class.
     * @param cefManager Reference to the CEF manager.
     */
    explicit CefApp(CefManager& cefManager);

    // CefApp methods
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return this; }
    void OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line) override;
    void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override;

    // CefBrowserProcessHandler methods
    void OnContextInitialized() override;
    void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) override;

    // CefRenderProcessHandler methods
    void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override;
    void OnWebKitInitialized() override;
    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
        CefProcessId source_process, CefRefPtr<CefProcessMessage> message) override;

private:
    // Required for IMPLEMENT_REFCOUNTING
    IMPLEMENT_REFCOUNTING(CefApp);

    CefManager& m_cefManager;         ///< Reference to the CEF manager
};

} // namespace poe