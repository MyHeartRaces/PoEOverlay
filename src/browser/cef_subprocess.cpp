#include <Windows.h>
#include <include/cef_app.h>
#include <iostream>

// Forward declarations
CefRefPtr<CefApp> CreateRendererApp();
CefRefPtr<CefApp> CreateOtherApp();

/**
 * @brief CEF subprocess entry point.
 * 
 * This is a separate executable that CEF launches for various process types 
 * (renderer, GPU, utility, etc.) as needed. It should be kept minimal
 * and only handle CEF initialization for the specific process type.
 */
int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    // Enable High-DPI support on Windows
    CefEnableHighDPISupport();

    // Create the main CefApp reference
    CefRefPtr<CefApp> app;

    // Parse command-line arguments
    CefMainArgs main_args(hInstance);
    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
    command_line->InitFromString(::GetCommandLineW());

    // Determine the process type
    std::string process_type = command_line->GetSwitchValue("type");
    if (process_type == "renderer" || process_type == "zygote") {
        // Create renderer process application
        app = CreateRendererApp();
    } else {
        // Create application for other process types
        app = CreateOtherApp();
    }

    // Execute the subprocess logic and return exit code
    return CefExecuteProcess(main_args, app, nullptr);
}

/**
 * @class RendererApp
 * @brief CEF App implementation for renderer processes.
 */
class RendererApp : public CefApp, public CefRenderProcessHandler {
public:
    RendererApp() {}

    // CefApp methods:
    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return this; }

    // CefRenderProcessHandler methods:
    void OnContextCreated(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefFrame> frame,
                          CefRefPtr<CefV8Context> context) override {
        // You can inject JavaScript code here if needed
    }

    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefProcessId source_process,
                                  CefRefPtr<CefProcessMessage> message) override {
        // Handle IPC messages from the browser process
        std::string message_name = message->GetName();
        if (message_name == "ping") {
            // Send a pong response
            CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("pong");
            frame->SendProcessMessage(PID_BROWSER, response);
            return true;
        }
        return false;
    }

private:
    IMPLEMENT_REFCOUNTING(RendererApp);
};

/**
 * @class OtherApp
 * @brief CEF App implementation for other process types.
 */
class OtherApp : public CefApp {
public:
    OtherApp() {}

private:
    IMPLEMENT_REFCOUNTING(OtherApp);
};

CefRefPtr<CefApp> CreateRendererApp() {
    return new RendererApp();
}

CefRefPtr<CefApp> CreateOtherApp() {
    return new OtherApp();
}