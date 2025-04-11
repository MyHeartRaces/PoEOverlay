#include "browser/RenderHandler.h"
#include "browser/CefManager.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"

#include <iostream>

namespace poe {

RenderHandler::RenderHandler(Application& app, CefManager& cefManager)
    : m_app(app)
    , m_cefManager(cefManager)
{
    Log(2, "RenderHandler created");
}

void RenderHandler::Resize(CefRefPtr<CefBrowser> browser, int width, int height)
{
    if (!browser)
    {
        return;
    }

    int browserId = browser->GetIdentifier();
    Log(2, "Resizing browser viewport: ID={}, Size={}x{}", browserId, width, height);
    
    {
        std::lock_guard<std::mutex> lock(m_viewportsMutex);
        
        auto it = m_viewports.find(browserId);
        if (it != m_viewports.end())
        {
            it->second.width = width;
            it->second.height = height;
        }
        else
        {
            ViewportInfo info;
            info.width = width;
            info.height = height;
            m_viewports[browserId] = info;
        }
    }
    
    // Notify browser of resize
    browser->GetHost()->WasResized();
}

bool RenderHandler::GetViewportSize(int browserId, int& width, int& height)
{
    std::lock_guard<std::mutex> lock(m_viewportsMutex);
    
    auto it = m_viewports.find(browserId);
    if (it != m_viewports.end())
    {
        width = it->second.width;
        height = it->second.height;
        return true;
    }
    
    return false;
}

bool RenderHandler::GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    if (!browser)
    {
        return false;
    }

    int width = 0, height = 0;
    if (GetViewportSize(browser->GetIdentifier(), width, height))
    {
        rect.Set(0, 0, width, height);
        return true;
    }
    
    return false;
}

void RenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    if (!browser)
    {
        rect.Set(0, 0, 800, 600);  // Default size
        return;
    }

    int width = 800, height = 600;  // Default size
    GetViewportSize(browser->GetIdentifier(), width, height);
    
    rect.Set(0, 0, width, height);
}

bool RenderHandler::GetScreenPoint(
    CefRefPtr<CefBrowser> browser,
    int viewX,
    int viewY,
    int& screenX,
    int& screenY)
{
    // In offscreen rendering, view coordinates are screen coordinates
    screenX = viewX;
    screenY = viewY;
    return true;
}

bool RenderHandler::GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info)
{
    // Set up screen info for the browser
    screen_info.device_scale_factor = 1.0f;
    
    // Get viewport size
    int width = 0, height = 0;
    if (GetViewportSize(browser->GetIdentifier(), width, height))
    {
        screen_info.rect.Set(0, 0, width, height);
        screen_info.available_rect = screen_info.rect;
        return true;
    }
    
    return false;
}

void RenderHandler::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
    int browserId = browser->GetIdentifier();
    Log(1, "Popup show: ID={}, Show={}", browserId, show);
    
    std::lock_guard<std::mutex> lock(m_viewportsMutex);
    
    auto it = m_viewports.find(browserId);
    if (it != m_viewports.end())
    {
        it->second.popupVisible = show;
    }
}

void RenderHandler::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect)
{
    int browserId = browser->GetIdentifier();
    Log(1, "Popup size: ID={}, Rect=[{},{},{},{}]", 
        browserId, rect.x, rect.y, rect.width, rect.height);
    
    std::lock_guard<std::mutex> lock(m_viewportsMutex);
    
    auto it = m_viewports.find(browserId);
    if (it != m_viewports.end())
    {
        it->second.popupRect = rect;
    }
}

void RenderHandler::OnPaint(
    CefRefPtr<CefBrowser> browser,
    PaintElementType type,
    const RectList& dirtyRects,
    const void* buffer,
    int width,
    int height)
{
    // For simplicity, just pass the entire buffer to the callback
    if (m_paintCallback)
    {
        m_paintCallback(browser, buffer, width, height, 0, 0);
    }
}

void RenderHandler::OnCursorChange(
    CefRefPtr<CefBrowser> browser,
    CefCursorHandle cursor,
    CursorType type,
    const CefCursorInfo& custom_cursor_info)
{
    if (m_cursorChangeCallback)
    {
        m_cursorChangeCallback(browser, cursor);
    }
}

bool RenderHandler::StartDragging(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDragData> drag_data,
    DragOperationsMask allowed_ops,
    int x,
    int y)
{
    // Return false to cancel the drag operation
    return false;
}

void RenderHandler::UpdateDragCursor(CefRefPtr<CefBrowser> browser, DragOperation operation)
{
    // No implementation needed
}

void RenderHandler::OnScrollOffsetChanged(CefRefPtr<CefBrowser> browser, double x, double y)
{
    // No implementation needed
}

template<typename... Args>
void RenderHandler::Log(int level, const std::string& fmt, const Args&... args)
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
        std::cerr << "RenderHandler log error: " << e.what() << std::endl;
    }
}

} // namespace poe