#pragma once

#include <Windows.h>
#include <dwmapi.h>
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include "window/monitor_info.h"
#include "core/Application.h"

namespace poe {

// Forward declarations
class Application;
class OverlayRenderer;
class AnimationManager;

/**
 * @class OverlayWindow
 * @brief Manages a non-intrusive overlay window for the application.
 * 
 * Handles window creation, composition, click-through capabilities,
 * multi-monitor support, and input management.
 */
class OverlayWindow {
public:
    /**
     * @enum WindowMode
     * @brief Defines the interaction modes for the overlay window.
     */
    enum class WindowMode {
        Interactive,    ///< Window receives mouse and keyboard input
        ClickThrough    ///< Window passes mouse events to underlying windows
    };

    /**
     * @struct WindowConfig
     * @brief Configuration parameters for the overlay window.
     */
    struct WindowConfig {
        std::wstring title = L"PoEOverlay";  ///< Window title
        int width = 800;                     ///< Initial window width
        int height = 600;                    ///< Initial window height
        float opacity = 1.0f;                ///< Window opacity (0.0-1.0)
        bool showOnStartup = false;          ///< Whether to show window on startup
        WindowMode initialMode = WindowMode::Interactive; ///< Initial interaction mode
    };

    /**
     * @brief Construct a new Overlay Window object
     * 
     * @param app Reference to the main application
     * @param config Configuration parameters
     */
    explicit OverlayWindow(Application& app, const WindowConfig& config = {});

    /**
     * @brief Destroy the Overlay Window object
     */
    ~OverlayWindow();

    // Non-copyable
    OverlayWindow(const OverlayWindow&) = delete;
    OverlayWindow& operator=(const OverlayWindow&) = delete;

    // Movable
    OverlayWindow(OverlayWindow&&) noexcept;
    OverlayWindow& operator=(OverlayWindow&&) noexcept;

    /**
     * @brief Create and initialize the window
     * 
     * @return true If window creation succeeded
     * @return false If window creation failed
     */
    bool Create();

    /**
     * @brief Set the window visibility
     * 
     * @param visible True to show the window, false to hide it
     * @param animate Whether to animate the transition
     */
    void SetVisible(bool visible, bool animate = false);

    /**
     * @brief Check if the window is currently visible
     * 
     * @return true If the window is visible
     * @return false If the window is hidden
     */
    bool IsVisible() const;

    /**
     * @brief Set the window interaction mode
     * 
     * @param mode The desired interaction mode
     */
    void SetMode(WindowMode mode);

    /**
     * @brief Get the current window interaction mode
     * 
     * @return WindowMode The current interaction mode
     */
    WindowMode GetMode() const;

    /**
     * @brief Set the window opacity
     * 
     * @param opacity Opacity value between 0.0 (transparent) and 1.0 (opaque)
     * @param animate Whether to animate the transition
     */
    void SetOpacity(float opacity, bool animate = false);

    /**
     * @brief Get the current window opacity
     * 
     * @return float The current opacity value
     */
    float GetOpacity() const;

    /**
     * @brief Set the window position and size
     * 
     * @param x X coordinate (left)
     * @param y Y coordinate (top)
     * @param width Window width
     * @param height Window height
     * @param repaint Whether to repaint the window immediately
     */
    void SetBounds(int x, int y, int width, int height, bool repaint = true);

    /**
     * @brief Get the window's current bounds
     * 
     * @return RECT The window's position and size
     */
    RECT GetBounds() const;

    /**
     * @brief Set a callback function for window events
     * 
     * @param callback Function to call when window events occur
     */
    using WindowEventCallback = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>;
    void SetEventCallback(WindowEventCallback callback);

    /**
     * @brief Get the native window handle
     * 
     * @return HWND The window handle
     */
    HWND GetHandle() const;

    /**
     * @brief Process window messages
     * 
     * @return true If the message loop should continue
     * @return false If the message loop should exit
     */
    bool ProcessMessages();

    /**
     * @brief Check if the window is positioned over the game window
     * 
     * @param gameWindowHandle The handle to the game window
     * @return true If the overlay is positioned over the game window
     * @return false Otherwise
     */
    bool IsOverlayingGame(HWND gameWindowHandle) const;

    /**
     * @brief Move the window to match the game window position
     * 
     * @param gameWindowHandle The handle to the game window
     * @return true If the window was repositioned successfully
     * @return false Otherwise
     */
    bool AlignWithGameWindow(HWND gameWindowHandle);

    /**
     * @brief Update the window state and animations
     */
    void Update();

    /**
     * @brief Check if the mouse cursor is near the window edge
     * 
     * @param x Mouse X coordinate
     * @param y Mouse Y coordinate
     * @param threshold Distance threshold in pixels
     * @return true If the mouse is near the edge
     * @return false Otherwise
     */
    bool IsMouseNearEdge(int x, int y, int threshold = 10) const;

private:
    /**
     * @brief Initialize DirectComposition for hardware-accelerated transparency
     * 
     * @return true If initialization succeeded
     * @return false If initialization failed
     */
    bool InitializeComposition();
    
    /**
     * @brief Log a message using the application logger
     * 
     * @tparam Args Variadic template for format arguments
     * @param level The log level (0=trace, 1=debug, 2=info, 3=warning, 4=error, 5=critical)
     * @param fmt Format string
     * @param args Format arguments
     */
    template<typename... Args>
    void Log(int level, const std::string& fmt, const Args&... args);

    /**
     * @brief Apply window styles to enable proper overlay behavior
     */
    void ApplyOverlayStyles();

    /**
     * @brief Set up window position and visual properties
     */
    void SetupWindowVisuals();

    /**
     * @brief Update the window's click-through status based on current mode
     */
    void UpdateClickThrough();
    
    /**
     * @brief Handles border highlighting based on mouse position
     */
    void UpdateBorderHighlight();

    /**
     * @brief Sets up animations
     */
    void SetupAnimations();

    /**
     * @brief Internal window procedure to handle window messages
     */
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    Application& m_app;                  ///< Reference to the main application
    WindowConfig m_config;                ///< Window configuration
    HWND m_windowHandle = nullptr;        ///< Window handle
    HINSTANCE m_instanceHandle = nullptr; ///< Application instance handle
    bool m_visible = false;               ///< Window visibility state
    WindowMode m_mode;                    ///< Current interaction mode
    float m_opacity = 1.0f;               ///< Current opacity value
    RECT m_bounds = {};                   ///< Current window bounds
    WindowEventCallback m_eventCallback;  ///< Custom event callback
    
    // Mouse tracking
    bool m_mouseTracking = false;         ///< Whether mouse tracking is active
    bool m_mouseNearEdge = false;         ///< Whether mouse is near window edge
    POINT m_lastMousePos = {0, 0};        ///< Last known mouse position
    
    // Advanced rendering
    std::unique_ptr<OverlayRenderer> m_renderer; ///< Overlay renderer
    std::unique_ptr<AnimationManager> m_animationManager; ///< Animation manager
    
    // DWM composition related fields
    bool m_compositionEnabled = false;    ///< Whether DWM composition is enabled
};

} // namespace poe