#include "browser/BrowserView.h"
#include "browser/BrowserHandler.h"
#include "browser/RenderHandler.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"

#include <iostream>

namespace poe {

BrowserView::BrowserView(
    Application& app,
    CefManager& cefManager,
    int width,
    int height,
    const std::string& url)
    : m_app(app)
    , m_cefManager(cefManager)
    , m_browser(nullptr)
    , m_width(width)
    , m_height(height)
    , m_visible(true)
    , m_currentUrl(url)
    , m_currentTitle("")
    , m_isLoading(false)
    , m_canGoBack(false)
    , m_canGoForward(false)
{
    Log(2, "BrowserView created with size {}x{}", width, height);
}

BrowserView::~BrowserView()
{
    Shutdown();
}

bool BrowserView::Initialize()
{
    if (m_browser)
    {
        Log(3, "BrowserView already initialized");
        return true;
    }

    try
    {
        Log(2, "Initializing BrowserView");
        
        // Setup browser handler callbacks
        auto browserHandler = m_cefManager.GetBrowserHandler();
        if (browserHandler)
        {
            browserHandler->SetBrowserCreatedCallback(
                [this](CefRefPtr<CefBrowser> browser) { OnBrowserCreated(browser); });
            
            browserHandler->SetBrowserCloseCallback(
                [this](CefRefPtr<CefBrowser> browser) { OnBrowserClose(browser); });
            
            browserHandler->SetLoadStateCallback(
                [this](CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) {
                    OnLoadingStateChange(isLoading, canGoBack, canGoForward);
                });
            
            browserHandler->SetTitleChangeCallback(
                [this](CefRefPtr<CefBrowser> browser, const std::string& title) {
                    OnTitleChange(title);
                });
            
            browserHandler->SetAddressChangeCallback(
                [this](CefRefPtr<CefBrowser> browser, const std::string& url) {
                    OnAddressChange(url);
                });
        }
        
        // Setup render handler callbacks
        auto renderHandler = m_cefManager.GetRenderHandler();
        if (renderHandler)
        {
            renderHandler->SetPaintCallback(
                [this](CefRefPtr<CefBrowser> browser, const void* buffer, int width, int height, int x, int y) {
                    OnPaint(buffer, width, height, x, y);
                });
        }
        
        // Create browser
        m_browser = m_cefManager.CreateBrowser(
            m_currentUrl,
            m_width,
            m_height,
            nullptr,
            true);
        
        if (!m_browser)
        {
            Log(4, "Failed to create browser");
            return false;
        }
        
        Log(2, "BrowserView initialized successfully");
        return true;
    }
    catch (const std::exception& ex)
    {
        m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "BrowserView");
        return false;
    }
}

void BrowserView::Shutdown()
{
    if (!m_browser)
    {
        return;
    }

    Log(2, "Shutting down BrowserView");
    
    // Close browser
    m_cefManager.CloseBrowser(m_browser, true);
    m_browser = nullptr;
}

void BrowserView::Navigate(const std::string& url)
{
    if (!m_browser)
    {
        Log(3, "Cannot navigate: browser not initialized");
        return;
    }

    Log(2, "Navigating to: {}", url);
    
    // Get the main frame and load the URL
    CefRefPtr<CefFrame> mainFrame = m_browser->GetMainFrame();
    if (mainFrame)
    {
        mainFrame->LoadURL(url);
    }
}

bool BrowserView::GoBack()
{
    if (!m_browser || !m_canGoBack)
    {
        return false;
    }

    m_browser->GoBack();
    return true;
}

bool BrowserView::GoForward()
{
    if (!m_browser || !m_canGoForward)
    {
        return false;
    }

    m_browser->GoForward();
    return true;
}

void BrowserView::Reload()
{
    if (!m_browser)
    {
        return;
    }

    m_browser->Reload();
}

void BrowserView::StopLoad()
{
    if (!m_browser)
    {
        return;
    }

    m_browser->StopLoad();
}

void BrowserView::Resize(int width, int height)
{
    if (m_width == width && m_height == height)
    {
        return;
    }

    Log(2, "Resizing browser view: {}x{}", width, height);
    
    m_width = width;
    m_height = height;
    
    // Resize the browser viewport
    if (m_browser)
    {
        auto renderHandler = m_cefManager.GetRenderHandler();
        if (renderHandler)
        {
            renderHandler->Resize(m_browser, width, height);
        }
    }
}

void BrowserView::SetVisible(bool visible)
{
    if (m_visible == visible)
    {
        return;
    }

    m_visible = visible;
    
    // Update browser visibility state
    if (m_browser)
    {
        m_browser->GetHost()->WasHidden(!visible);
    }
}

void BrowserView::OnMouseMove(int x, int y, uint32_t modifiers)
{
    if (!m_browser || !m_visible)
    {
        return;
    }

    // Convert mouse coordinates to browser coordinates
    CefMouseEvent event;
    event.x = x;
    event.y = y;
    event.modifiers = modifiers;
    
    // Send mouse move event
    m_browser->GetHost()->SendMouseMoveEvent(event, false);
}

void BrowserView::OnMouseButton(int x, int y, int button, uint32_t modifiers, bool isDown)
{
    if (!m_browser || !m_visible)
    {
        return;
    }

    // Convert mouse coordinates to browser coordinates
    CefMouseEvent event;
    event.x = x;
    event.y = y;
    event.modifiers = modifiers;
    
    // Map button to CEF button type
    CefBrowserHost::MouseButtonType cefButton;
    switch (button)
    {
        case 0: cefButton = MBT_LEFT; break;
        case 1: cefButton = MBT_MIDDLE; break;
        case 2: cefButton = MBT_RIGHT; break;
        default: return; // Unsupported button
    }
    
    // Send mouse button event
    m_browser->GetHost()->SendMouseClickEvent(event, cefButton, !isDown, 1);
}

void BrowserView::OnMouseWheel(int x, int y, int deltaX, int deltaY)
{
    if (!m_browser || !m_visible)
    {
        return;
    }

    // Convert mouse coordinates to browser coordinates
    CefMouseEvent event;
    event.x = x;
    event.y = y;
    event.modifiers = 0;
    
    // Send mouse wheel event
    m_browser->GetHost()->SendMouseWheelEvent(event, deltaX, deltaY);
}

void BrowserView::OnKey(int key, uint32_t modifiers, bool isDown)
{
    if (!m_browser || !m_visible)
    {
        return;
    }

    // Create key event
    CefKeyEvent event;
    event.windows_key_code = key;
    event.native_key_code = key;
    event.modifiers = modifiers;
    event.type = isDown ? KEYEVENT_KEYDOWN : KEYEVENT_KEYUP;
    
    // Send key event
    m_browser->GetHost()->SendKeyEvent(event);
}

void BrowserView::OnChar(unsigned int character, uint32_t modifiers)
{
    if (!m_browser || !m_visible)
    {
        return;
    }

    // Create key event
    CefKeyEvent event;
    event.windows_key_code = character;
    event.native_key_code = character;
    event.modifiers = modifiers;
    event.type = KEYEVENT_CHAR;
    
    // Send key event
    m_browser->GetHost()->SendKeyEvent(event);
}

std::string BrowserView::GetCurrentUrl() const
{
    return m_currentUrl;
}

std::string BrowserView::GetCurrentTitle() const
{
    return m_currentTitle;
}

bool BrowserView::IsLoading() const
{
    return m_isLoading;
}

bool BrowserView::CanGoBack() const
{
    return m_canGoBack;
}

bool BrowserView::CanGoForward() const
{
    return m_canGoForward;
}

void BrowserView::OnBrowserCreated(CefRefPtr<CefBrowser> browser)
{
    // Only handle our browser
    if (!m_browser || browser->GetIdentifier() != m_browser->GetIdentifier())
    {
        return;
    }

    Log(2, "Browser created successfully");
}

void BrowserView::OnBrowserClose(CefRefPtr<CefBrowser> browser)
{
    // Only handle our browser
    if (!m_browser || browser->GetIdentifier() != m_browser->GetIdentifier())
    {
        return;
    }

    Log(2, "Browser closed");
    m_browser = nullptr;
}

void BrowserView::OnLoadingStateChange(bool isLoading, bool canGoBack, bool canGoForward)
{
    // Update state
    m_isLoading = isLoading;
    m_canGoBack = canGoBack;
    m_canGoForward = canGoForward;
    
    Log(1, "Loading state changed: isLoading={}, canGoBack={}, canGoForward={}",
        isLoading, canGoBack, canGoForward);
    
    // Notify callback if set
    if (m_navigationStateCallback)
    {
        m_navigationStateCallback(isLoading, canGoBack, canGoForward);
    }
}

void BrowserView::OnTitleChange(const std::string& title)
{
    // Update title
    m_currentTitle = title;
    
    Log(1, "Title changed: {}", title);
    
    // Notify callback if set
    if (m_titleChangeCallback)
    {
        m_titleChangeCallback(title);
    }
}

void BrowserView::OnAddressChange(const std::string& url)
{
    // Update URL
    m_currentUrl = url;
    
    Log(1, "Address changed: {}", url);
    
    // Notify callback if set
    if (m_addressChangeCallback)
    {
        m_addressChangeCallback(url);
    }
}

void BrowserView::OnPaint(const void* buffer, int width, int height, int x, int y)
{
    // Ignore if not visible
    if (!m_visible)
    {
        return;
    }
    
    // Notify callback if set
    if (m_paintCallback)
    {
        m_paintCallback(buffer, width, height);
    }
}

template<typename... Args>
void BrowserView::Log(int level, const std::string& fmt, const Args&... args)
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
        std::cerr << "BrowserView log error: " << e.what() << std::endl;
    }
}

} // namespace poe