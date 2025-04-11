#include "browser/BrowserClient.h"
#include "browser/BrowserHandler.h"
#include "browser/RenderHandler.h"
#include "browser/CefManager.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"

#include <iostream>

namespace poe {

BrowserClient::BrowserClient(
    Application& app,
    CefManager& cefManager,
    BrowserHandler* browserHandler,
    RenderHandler* renderHandler)
    : m_app(app)
    , m_cefManager(cefManager)
    , m_browserHandler(browserHandler)
    , m_renderHandler(renderHandler)
{
    Log(2, "BrowserClient created");
}

CefRefPtr<CefLifeSpanHandler> BrowserClient::GetLifeSpanHandler()
{
    return m_browserHandler;
}

CefRefPtr<CefLoadHandler> BrowserClient::GetLoadHandler()
{
    return m_browserHandler;
}

CefRefPtr<CefDisplayHandler> BrowserClient::GetDisplayHandler()
{
    return m_browserHandler;
}

CefRefPtr<CefContextMenuHandler> BrowserClient::GetContextMenuHandler()
{
    return m_browserHandler;
}

CefRefPtr<CefRenderHandler> BrowserClient::GetRenderHandler()
{
    return m_renderHandler;
}

bool BrowserClient::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message)
{
    // Handle messages from the renderer process
    std::string messageName = message->GetName().ToString();
    Log(1, "Received process message: {} from process: {}", 
        messageName, source_process == PID_RENDERER ? "renderer" : "unknown");
    
    // Example handling of a "ping" message
    if (messageName == "ping")
    {
        // Send a "pong" response
        CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("pong");
        frame->SendProcessMessage(PID_RENDERER, response);
        return true;
    }
    
    return false;
}

template<typename... Args>
void BrowserClient::Log(int level, const std::string& fmt, const Args&... args)
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
        std::cerr << "BrowserClient log error: " << e.what() << std::endl;
    }
}

} // namespace poe