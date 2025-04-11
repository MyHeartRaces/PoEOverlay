#include "browser/CefApp.h"
#include "browser/CefManager.h"
#include <iostream>

namespace poe {

CefApp::CefApp(CefManager& cefManager)
    : m_cefManager(cefManager)
{
}

void CefApp::OnBeforeCommandLineProcessing(
    const CefString& process_type,
    CefRefPtr<CefCommandLine> command_line)
{
    // Modify command line arguments if needed
    // Note: This is called on all processes (browser, renderer, etc.)
    if (process_type.empty())
    {
        // This is the browser process
        // Disable sandbox for simplicity
        command_line->AppendSwitch("no-sandbox");
        
        // Disable GPU acceleration to reduce resource usage
        command_line->AppendSwitch("disable-gpu");
        command_line->AppendSwitch("disable-gpu-compositing");
        
        // Disable unnecessary features
        command_line->AppendSwitch("disable-extensions");
        command_line->AppendSwitch("disable-pinch");
    }
}

void CefApp::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar)
{
    // Register custom scheme for poe://
    registrar->AddCustomScheme("poe", CEF_SCHEME_OPTION_STANDARD);
}

void CefApp::OnContextInitialized()
{
    // The CEF context has been initialized
    // This is called in the browser process when CEF is ready
    std::cout << "CEF context initialized" << std::endl;
}

void CefApp::OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line)
{
    // This is called before a child process is launched
    std::string processType = command_line->GetSwitchValue("type").ToString();
    if (!processType.empty())
    {
        std::cout << "Launching CEF child process: " << processType << std::endl;
    }
}

void CefApp::OnContextCreated(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefV8Context> context)
{
    // A new JavaScript context has been created in the renderer process
    // This is a good place to inject custom JavaScript objects
}

void CefApp::OnWebKitInitialized()
{
    // WebKit has been initialized in the renderer process
    // This is a good place to register custom JavaScript extensions
}

bool CefApp::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message)
{
    // Handle messages from browser process (in renderer process)
    // or from renderer process (in browser process)
    std::string messageName = message->GetName().ToString();
    
    if (messageName == "pong")
    {
        // Example handling of a "pong" message
        std::cout << "Received pong message from process: " 
                  << (source_process == PID_BROWSER ? "browser" : "unknown") << std::endl;
        return true;
    }
    
    return false;
}

} // namespace poe