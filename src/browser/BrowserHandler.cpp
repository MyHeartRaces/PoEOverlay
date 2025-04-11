#include "browser/BrowserHandler.h"
#include "browser/CefManager.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"

#include <iostream>

namespace poe {

BrowserHandler::BrowserHandler(Application& app, CefManager& cefManager)
    : m_app(app)
    , m_cefManager(cefManager)
{
    Log(2, "BrowserHandler created");
}

bool BrowserHandler::OnBeforePopup(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    const CefString& target_url,
    const CefString& target_frame_name,
    CefLifeSpanHandler::WindowOpenDisposition target_disposition,
    bool user_gesture,
    const CefPopupFeatures& popupFeatures,
    CefWindowInfo& windowInfo,
    CefRefPtr<CefClient>& client,
    CefBrowserSettings& settings,
    CefRefPtr<CefDictionaryValue>& extra_info,
    bool* no_javascript_access)
{
    // Handle popup by creating a new browser
    std::string url = target_url.ToString();
    Log(2, "OnBeforePopup: {} (disposition: {})", url, static_cast<int>(target_disposition));
    
    // By default, we'll handle all popups ourselves
    // Return true to cancel the popup
    return true;
}

void BrowserHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    Log(2, "Browser created: ID={}", browser->GetIdentifier());
    
    // Store browser data
    {
        std::lock_guard<std::mutex> lock(m_browserDataMutex);
        int browserId = browser->GetIdentifier();
        m_browserData[browserId] = BrowserData();
    }
    
    // Notify callback if set
    if (m_browserCreatedCallback)
    {
        m_browserCreatedCallback(browser);
    }
}

bool BrowserHandler::DoClose(CefRefPtr<CefBrowser> browser)
{
    Log(2, "Browser closing: ID={}", browser->GetIdentifier());
    
    // Return false to allow the close to proceed
    return false;
}

void BrowserHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    int browserId = browser->GetIdentifier();
    Log(2, "Browser closed: ID={}", browserId);
    
    // Remove browser data
    {
        std::lock_guard<std::mutex> lock(m_browserDataMutex);
        m_browserData.erase(browserId);
    }
    
    // Notify callback if set
    if (m_browserCloseCallback)
    {
        m_browserCloseCallback(browser);
    }
}

void BrowserHandler::OnLoadingStateChange(
    CefRefPtr<CefBrowser> browser,
    bool isLoading,
    bool canGoBack,
    bool canGoForward)
{
    int browserId = browser->GetIdentifier();
    Log(1, "Browser loading state changed: ID={}, isLoading={}, canGoBack={}, canGoForward={}",
        browserId, isLoading, canGoBack, canGoForward);
    
    // Update browser data
    {
        std::lock_guard<std::mutex> lock(m_browserDataMutex);
        auto it = m_browserData.find(browserId);
        if (it != m_browserData.end())
        {
            it->second.isLoading = isLoading;
            it->second.canGoBack = canGoBack;
            it->second.canGoForward = canGoForward;
        }
    }
    
    // Notify callback if set
    if (m_loadStateCallback)
    {
        m_loadStateCallback(browser, isLoading, canGoBack, canGoForward);
    }
}

void BrowserHandler::OnLoadError(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    ErrorCode errorCode,
    const CefString& errorText,
    const CefString& failedUrl)
{
    // Only log errors for the main frame
    if (frame->IsMain())
    {
        std::string url = failedUrl.ToString();
        std::string error = errorText.ToString();
        Log(3, "Browser load error: URL={}, Error={} (Code: {})",
            url, error, static_cast<int>(errorCode));
        
        // Display an error page
        std::stringstream ss;
        ss << "<html><body style=\"background-color: #f1f1f1; font-family: Arial, sans-serif; color: #333; padding: 20px;\">"
           << "<h2>Page Load Error</h2>"
           << "<p>Failed to load: <span style=\"color: #777;\">" << url << "</span></p>"
           << "<p>Error: " << error << " (Code: " << static_cast<int>(errorCode) << ")</p>"
           << "<a href=\"" << url << "\" style=\"color: #3498db;\">Try again</a>"
           << "</body></html>";
        
        frame->LoadString(ss.str(), failedUrl);
    }
}

void BrowserHandler::OnTitleChange(
    CefRefPtr<CefBrowser> browser,
    const CefString& title)
{
    int browserId = browser->GetIdentifier();
    std::string titleStr = title.ToString();
    Log(1, "Browser title changed: ID={}, Title={}", browserId, titleStr);
    
    // Update browser data
    {
        std::lock_guard<std::mutex> lock(m_browserDataMutex);
        auto it = m_browserData.find(browserId);
        if (it != m_browserData.end())
        {
            it->second.title = titleStr;
        }
    }
    
    // Notify callback if set
    if (m_titleChangeCallback)
    {
        m_titleChangeCallback(browser, titleStr);
    }
}

void BrowserHandler::OnAddressChange(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    const CefString& url)
{
    // Only update for the main frame
    if (frame->IsMain())
    {
        int browserId = browser->GetIdentifier();
        std::string urlStr = url.ToString();
        Log(1, "Browser address changed: ID={}, URL={}", browserId, urlStr);
        
        // Update browser data
        {
            std::lock_guard<std::mutex> lock(m_browserDataMutex);
            auto it = m_browserData.find(browserId);
            if (it != m_browserData.end())
            {
                it->second.url = urlStr;
            }
        }
        
        // Notify callback if set
        if (m_addressChangeCallback)
        {
            m_addressChangeCallback(browser, urlStr);
        }
    }
}

void BrowserHandler::OnStatusMessage(
    CefRefPtr<CefBrowser> browser,
    const CefString& value)
{
    std::string message = value.ToString();
    if (!message.empty())
    {
        Log(1, "Browser status message: ID={}, Message={}", browser->GetIdentifier(), message);
        
        // Notify callback if set
        if (m_statusMessageCallback)
        {
            m_statusMessageCallback(browser, message);
        }
    }
}

bool BrowserHandler::OnConsoleMessage(
    CefRefPtr<CefBrowser> browser,
    cef_log_severity_t level,
    const CefString& message,
    const CefString& source,
    int line)
{
    // Convert CEF log level to our log level
    int logLevel;
    switch (level)
    {
        case LOGSEVERITY_VERBOSE: logLevel = 0; break;
        case LOGSEVERITY_DEBUG: logLevel = 1; break;
        case LOGSEVERITY_INFO: logLevel = 2; break;
        case LOGSEVERITY_WARNING: logLevel = 3; break;
        case LOGSEVERITY_ERROR: logLevel = 4; break;
        case LOGSEVERITY_FATAL: logLevel = 5; break;
        default: logLevel = 2; break;
    }
    
    // Log the console message
    std::string messageStr = message.ToString();
    std::string sourceStr = source.ToString();
    
    Log(logLevel, "Browser console: [{}:{}] {}", sourceStr, line, messageStr);
    
    // Return true to indicate the message was handled
    return true;
}

void BrowserHandler::OnBeforeContextMenu(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefContextMenuParams> params,
    CefRefPtr<CefMenuModel> model)
{
    // Customize the context menu
    // Clear the default menu
    model->Clear();
    
    // Add custom menu items
    if (params->GetTypeFlags() & (CM_TYPEFLAG_PAGE | CM_TYPEFLAG_FRAME))
    {
        model->AddItem(1, "Back");
        model->AddItem(2, "Forward");
        model->AddItem(3, "Reload");
        model->AddSeparator();
        model->AddItem(4, "Copy");
        
        // If there is text selection
        if (params->GetTypeFlags() & CM_TYPEFLAG_SELECTION)
        {
            model->AddItem(5, "Copy Selection");
        }
        
        // If there is a link
        if (params->GetTypeFlags() & CM_TYPEFLAG_LINK)
        {
            model->AddSeparator();
            model->AddItem(6, "Open Link in New Tab");
            model->AddItem(7, "Copy Link Address");
        }
    }
}

bool BrowserHandler::OnContextMenuCommand(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefContextMenuParams> params,
    int command_id,
    EventFlags event_flags)
{
    // Handle custom context menu commands
    switch (command_id)
    {
        case 1: // Back
            if (browser->CanGoBack())
                browser->GoBack();
            return true;
            
        case 2: // Forward
            if (browser->CanGoForward())
                browser->GoForward();
            return true;
            
        case 3: // Reload
            browser->Reload();
            return true;
            
        case 4: // Copy
            frame->Copy();
            return true;
            
        case 5: // Copy Selection
            frame->Copy();
            return true;
            
        case 6: // Open Link in New Tab
            // Create a new browser with the link URL
            m_cefManager.CreateBrowser(
                params->GetLinkUrl().ToString(),
                800, 600,
                nullptr,
                true
            );
            return true;
            
        case 7: // Copy Link Address
            // Copy link to clipboard
            return true;
    }
    
    return false;
}

template<typename... Args>
void BrowserHandler::Log(int level, const std::string& fmt, const Args&... args)
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
        std::cerr << "BrowserHandler log error: " << e.what() << std::endl;
    }
}

} // namespace poe