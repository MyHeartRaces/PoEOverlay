#pragma once

#include <Windows.h>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <include/cef_render_handler.h>
#include "core/Application.h"

namespace poe {

// Forward declarations
class Application;
class CefManager;

/**
 * @class RenderHandler
 * @brief Handles rendering of browser content to an off-screen buffer.
 * 
 * This class implements the CEF render handler interface to support
 * off-screen rendering of browser content, which is essential for
 * integrating web content into the overlay.
 */
class RenderHandler : public CefRenderHandler {
public:
    /**
     * @brief Type definition for paint callback function.
     */
    using PaintCallback = std::function<void(CefRefPtr<CefBrowser>, const void*, int, int, int, int)>;

    /**
     * @brief Type definition for cursor change callback function.
     */
    using CursorChangeCallback = std::function<void(CefRefPtr<CefBrowser>, CefCursorHandle)>;

    /**
     * @brief Constructor for the RenderHandler class.
     * @param app Reference to the main application instance.
     * @param cefManager Reference to the CEF manager.
     */
    RenderHandler(Application& app, CefManager& cefManager);

    /**
     * @brief Sets the callback function for paint events.
     * @param callback The callback function.
     */
    void SetPaintCallback(PaintCallback callback) { m_paintCallback = callback; }

    /**
     * @brief Sets the callback function for cursor change events.
     * @param callback The callback function.
     */
    void SetCursorChangeCallback(CursorChangeCallback callback) { m_cursorChangeCallback = callback; }

    /**
     * @brief Resizes the browser view.
     * @param browser The browser to resize.
     * @param width The new width.
     * @param height The new height.
     */
    void Resize(CefRefPtr<CefBrowser> browser, int width, int height);

    /**
     * @brief Gets the viewport size for a browser.
     * @param browserId The browser ID.
     * @param width Output parameter for the viewport width.
     * @param height Output parameter for the viewport height.
     * @return True if the viewport size was retrieved, false otherwise.
     */
    bool GetViewportSize(int browserId, int& width, int& height);

    // CefRenderHandler methods
    bool GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
    bool GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY) override;
    bool GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info) override;
    void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) override;
    void OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect) override;
    void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects,
        const void* buffer, int width, int height) override;
    void OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor, 
        CursorType type, const CefCursorInfo& custom_cursor_info) override;
    bool StartDragging(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDragData> drag_data,
        DragOperationsMask allowed_ops, int x, int y) override;
    void UpdateDragCursor(CefRefPtr<CefBrowser> browser, DragOperation operation) override;
    void OnScrollOffsetChanged(CefRefPtr<CefBrowser> browser, double x, double y) override;

private:
    /**
     * @struct ViewportInfo
     * @brief Stores information about a browser viewport.
     */
    struct ViewportInfo {
        int width = 800;     ///< Viewport width
        int height = 600;    ///< Viewport height
        bool popupVisible = false; ///< Whether a popup is visible
        CefRect popupRect;   ///< Popup rectangle
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
    IMPLEMENT_REFCOUNTING(RenderHandler);

    Application& m_app;                             ///< Reference to the main application
    CefManager& m_cefManager;                       ///< Reference to the CEF manager
    
    PaintCallback m_paintCallback;                  ///< Callback for paint events
    CursorChangeCallback m_cursorChangeCallback;    ///< Callback for cursor change events
    
    std::unordered_map<int, ViewportInfo> m_viewports; ///< Map of browser ID to viewport info
    std::mutex m_viewportsMutex;                    ///< Mutex for thread-safe access to viewports
};

} // namespace poe