#pragma once

#include <Windows.h>
#include <memory>
#include <string>
#include <functional>
#include <include/cef_browser.h>
#include "core/Application.h"
#include "browser/CefManager.h"

namespace poe {

/**
 * @class BrowserView
 * @brief Integrates a CEF browser into the overlay window.
 * 
 * This class manages a browser instance and handles its rendering,
 * providing a view component that can be embedded in the overlay UI.
 */
class BrowserView {
public:
    /**
     * @brief Constructor for the BrowserView class.
     * @param app Reference to the main application instance.
     * @param cefManager Reference to the CEF manager.
     * @param width Initial width of the browser view.
     * @param height Initial height of the browser view.
     * @param url Initial URL to navigate to.
     */
    BrowserView(
        Application& app,
        CefManager& cefManager,
        int width = 800,
        int height = 600,
        const std::string& url = "about:blank"
    );

    /**
     * @brief Destructor for the BrowserView class.
     */
    ~BrowserView();

    /**
     * @brief Initializes the browser view.
     * @return True if initialization succeeded, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Shuts down the browser view.
     */
    void Shutdown();

    /**
     * @brief Navigates to a URL.
     * @param url The URL to navigate to.
     */
    void Navigate(const std::string& url);

    /**
     * @brief Goes back in browser history.
     * @return True if navigation succeeded, false otherwise.
     */
    bool GoBack();

    /**
     * @brief Goes forward in browser history.
     * @return True if navigation succeeded, false otherwise.
     */
    bool GoForward();

    /**
     * @brief Reloads the current page.
     */
    void Reload();

    /**
     * @brief Stops loading the current page.
     */
    void StopLoad();

    /**
     * @brief Resizes the browser view.
     * @param width New width.
     * @param height New height.
     */
    void Resize(int width, int height);

    /**
     * @brief Sets the visibility of the browser view.
     * @param visible Whether the view should be visible.
     */
    void SetVisible(bool visible);

    /**
     * @brief Handles a mouse move event.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param modifiers Key modifiers.
     */
    void OnMouseMove(int x, int y, uint32_t modifiers);

    /**
     * @brief Handles a mouse click event.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param button Button that was clicked.
     * @param modifiers Key modifiers.
     * @param isDown Whether the button is down.
     */
    void OnMouseButton(int x, int y, int button, uint32_t modifiers, bool isDown);

    /**
     * @brief Handles a mouse wheel event.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param deltaX Horizontal scroll amount.
     * @param deltaY Vertical scroll amount.
     */
    void OnMouseWheel(int x, int y, int deltaX, int deltaY);

    /**
     * @brief Handles a key event.
     * @param key Key code.
     * @param modifiers Key modifiers.
     * @param isDown Whether the key is down.
     */
    void OnKey(int key, uint32_t modifiers, bool isDown);

    /**
     * @brief Handles a character input event.
     * @param character Character code.
     * @param modifiers Key modifiers.
     */
    void OnChar(unsigned int character, uint32_t modifiers);

    /**
     * @brief Gets the width of the browser view.
     * @return The width.
     */
    int GetWidth() const { return m_width; }

    /**
     * @brief Gets the height of the browser view.
     * @return The height.
     */
    int GetHeight() const { return m_height; }

    /**
     * @brief Gets the current URL.
     * @return The current URL.
     */
    std::string GetCurrentUrl() const;

    /**
     * @brief Gets the current title.
     * @return The current title.
     */
    std::string GetCurrentTitle() const;

    /**
     * @brief Checks if the browser is currently loading.
     * @return True if loading, false otherwise.
     */
    bool IsLoading() const;

    /**
     * @brief Checks if the browser can go back.
     * @return True if it can go back, false otherwise.
     */
    bool CanGoBack() const;

    /**
     * @brief Checks if the browser can go forward.
     * @return True if it can go forward, false otherwise.
     */
    bool CanGoForward() const;

    /**
     * @brief Sets the callback for navigation state changes.
     * @param callback The callback function.
     */
    void SetNavigationStateCallback(std::function<void(bool, bool, bool)> callback) { 
        m_navigationStateCallback = callback; 
    }

    /**
     * @brief Sets the callback for title changes.
     * @param callback The callback function.
     */
    void SetTitleChangeCallback(std::function<void(const std::string&)> callback) { 
        m_titleChangeCallback = callback; 
    }

    /**
     * @brief Sets the callback for address changes.
     * @param callback The callback function.
     */
    void SetAddressChangeCallback(std::function<void(const std::string&)> callback) { 
        m_addressChangeCallback = callback; 
    }

    /**
     * @brief Sets the callback for paint events.
     * @param callback The callback function.
     */
    void SetPaintCallback(std::function<void(const void*, int, int)> callback) { 
        m_paintCallback = callback; 
    }

private:
    /**
     * @brief Handles browser creation.
     * @param browser The created browser.
     */
    void OnBrowserCreated(CefRefPtr<CefBrowser> browser);

    /**
     * @brief Handles browser close.
     * @param browser The browser being closed.
     */
    void OnBrowserClose(CefRefPtr<CefBrowser> browser);

    /**
     * @brief Handles loading state changes.
     * @param isLoading Whether the browser is loading.
     * @param canGoBack Whether the browser can go back.
     * @param canGoForward Whether the browser can go forward.
     */
    void OnLoadingStateChange(bool isLoading, bool canGoBack, bool canGoForward);

    /**
     * @brief Handles title changes.
     * @param title The new title.
     */
    void OnTitleChange(const std::string& title);

    /**
     * @brief Handles address changes.
     * @param url The new URL.
     */
    void OnAddressChange(const std::string& url);

    /**
     * @brief Handles paint events.
     * @param buffer The pixel buffer.
     * @param width The width of the buffer.
     * @param height The height of the buffer.
     * @param x The x coordinate of the painted area.
     * @param y The y coordinate of the painted area.
     */
    void OnPaint(const void* buffer, int width, int height, int x, int y);

    /**
     * @brief Log a message using the application logger.
     * @tparam Args Variadic template for format arguments.
     * @param level The log level (0=trace, 1=debug, 2=info, 3=warning, 4=error, 5=critical).
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    void Log(int level, const std::string& fmt, const Args&... args);

    Application& m_app;                            ///< Reference to the main application
    CefManager& m_cefManager;                      ///< Reference to the CEF manager
    
    CefRefPtr<CefBrowser> m_browser;               ///< Browser instance
    
    int m_width;                                   ///< Width of the browser view
    int m_height;                                  ///< Height of the browser view
    bool m_visible;                                ///< Whether the browser view is visible
    
    // Current browser state
    std::string m_currentUrl;                      ///< Current URL
    std::string m_currentTitle;                    ///< Current title
    bool m_isLoading;                              ///< Whether the browser is loading
    bool m_canGoBack;                              ///< Whether the browser can go back
    bool m_canGoForward;                           ///< Whether the browser can go forward
    
    // Callbacks
    std::function<void(bool, bool, bool)> m_navigationStateCallback; ///< Callback for navigation state changes
    std::function<void(const std::string&)> m_titleChangeCallback;   ///< Callback for title changes
    std::function<void(const std::string&)> m_addressChangeCallback; ///< Callback for address changes
    std::function<void(const void*, int, int)> m_paintCallback;      ///< Callback for paint events
};

} // namespace poe