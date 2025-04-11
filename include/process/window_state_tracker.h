#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <unordered_map>
#include "core/Application.h"

namespace poe {

// Forward declarations
class Application;

/**
 * @enum WindowState
 * @brief Enumeration of window states.
 */
enum class WindowState {
    Normal,      ///< Window is in normal state
    Minimized,   ///< Window is minimized
    Maximized,   ///< Window is maximized
    Hidden,      ///< Window is hidden
    Invalid      ///< Window is invalid or closed
};

/**
 * @struct WindowStateInfo
 * @brief Information about a window's state.
 */
struct WindowStateInfo {
    HWND handle = nullptr;        ///< Window handle
    std::wstring title;           ///< Window title
    WindowState state = WindowState::Invalid; ///< Current window state
    bool hasFocus = false;        ///< Whether the window has focus
    RECT bounds = {0};            ///< Window bounds
    DWORD processId = 0;          ///< Process ID that owns the window
    bool isTopmost = false;       ///< Whether the window is topmost
};

/**
 * @class WindowStateTracker
 * @brief Tracks the state of windows.
 * 
 * This class is responsible for tracking window states, detecting
 * position and size changes, and monitoring window order changes.
 */
class WindowStateTracker {
public:
    /**
     * @brief Type definition for window state change callbacks.
     */
    using WindowStateCallback = std::function<void(const WindowStateInfo&, const WindowStateInfo&)>;

    /**
     * @brief Constructor for the WindowStateTracker class.
     * @param app Reference to the main application instance.
     */
    explicit WindowStateTracker(Application& app);

    /**
     * @brief Destructor for the WindowStateTracker class.
     */
    ~WindowStateTracker();

    // Non-copyable
    WindowStateTracker(const WindowStateTracker&) = delete;
    WindowStateTracker& operator=(const WindowStateTracker&) = delete;

    // Movable
    WindowStateTracker(WindowStateTracker&&) noexcept;
    WindowStateTracker& operator=(WindowStateTracker&&) noexcept;

    /**
     * @brief Initializes the window state tracker.
     * @return True if initialization succeeded, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Shuts down the window state tracker.
     */
    void Shutdown();

    /**
     * @brief Adds a window to be tracked.
     * @param handle The window handle to track.
     * @return True if the window was added, false if it was already tracked or invalid.
     */
    bool AddWindow(HWND handle);

    /**
     * @brief Removes a window from tracking.
     * @param handle The window handle to stop tracking.
     * @return True if the window was removed, false if it wasn't tracked.
     */
    bool RemoveWindow(HWND handle);

    /**
     * @brief Gets the current state of a window.
     * @param handle The window handle to get state for.
     * @return Window state information, or info with state Invalid if not tracked.
     */
    WindowStateInfo GetWindowState(HWND handle) const;

    /**
     * @brief Checks if a window is currently tracked.
     * @param handle The window handle to check.
     * @return True if the window is tracked, false otherwise.
     */
    bool IsWindowTracked(HWND handle) const;

    /**
     * @brief Updates the state of all tracked windows.
     * This should be called periodically to update window states.
     */
    void Update();

    /**
     * @brief Registers a callback for window state changes.
     * @param callback The callback function to register.
     * @return ID of the callback for later removal.
     */
    size_t RegisterStateCallback(const WindowStateCallback& callback);

    /**
     * @brief Unregisters a window state callback.
     * @param callbackId ID of the callback to unregister.
     * @return True if the callback was found and removed, false otherwise.
     */
    bool UnregisterStateCallback(size_t callbackId);

private:
    /**
     * @brief Internal structure for storing callbacks with their IDs.
     */
    struct CallbackEntry {
        size_t id;
        WindowStateCallback callback;
    };

    /**
     * @brief Updates the state of a specific window.
     * @param handle The window handle to update.
     * @return True if the window state was updated, false otherwise.
     */
    bool UpdateWindowState(HWND handle);

    /**
     * @brief Retrieves current information for a window.
     * @param handle The window handle to get info for.
     * @return Window state information.
     */
    WindowStateInfo GetWindowInfo(HWND handle) const;

    /**
     * @brief Notifies all registered callbacks about a state change.
     * @param oldInfo The previous window state information.
     * @param newInfo The new window state information.
     */
    void NotifyStateChange(const WindowStateInfo& oldInfo, const WindowStateInfo& newInfo);

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
    bool m_initialized;                            ///< Whether the tracker is initialized
    
    std::unordered_map<HWND, WindowStateInfo> m_windowStates; ///< Map of tracked window states
    mutable std::mutex m_windowStatesMutex;        ///< Mutex for thread-safe access to window states
    
    std::vector<CallbackEntry> m_callbacks;        ///< Registered state callbacks
    std::mutex m_callbacksMutex;                   ///< Mutex for thread-safe access to callbacks
    size_t m_nextCallbackId;                       ///< Counter for generating callback IDs
};

} // namespace poe