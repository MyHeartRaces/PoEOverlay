#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include "core/Application.h"

namespace poe {

// Forward declarations
class Application;

/**
 * @struct FocusChangeInfo
 * @brief Information about a focus change event.
 */
struct FocusChangeInfo {
    HWND previousWindow = nullptr;    ///< Previously focused window
    HWND currentWindow = nullptr;     ///< Currently focused window
    std::wstring previousTitle;       ///< Title of previously focused window
    std::wstring currentTitle;        ///< Title of currently focused window
    DWORD previousProcessId = 0;      ///< Process ID of previously focused window
    DWORD currentProcessId = 0;       ///< Process ID of currently focused window
    std::chrono::steady_clock::time_point timestamp; ///< Time of focus change
};

/**
 * @class FocusTracker
 * @brief Tracks window focus changes in the system.
 * 
 * This class is responsible for monitoring focus changes between
 * windows and notifying registered callbacks when focus changes.
 */
class FocusTracker {
public:
    /**
     * @brief Type definition for focus change callbacks.
     */
    using FocusChangeCallback = std::function<void(const FocusChangeInfo&)>;

    /**
     * @brief Constructor for the FocusTracker class.
     * @param app Reference to the main application instance.
     */
    explicit FocusTracker(Application& app);

    /**
     * @brief Destructor for the FocusTracker class.
     */
    ~FocusTracker();

    // Non-copyable
    FocusTracker(const FocusTracker&) = delete;
    FocusTracker& operator=(const FocusTracker&) = delete;

    // Movable
    FocusTracker(FocusTracker&&) noexcept;
    FocusTracker& operator=(FocusTracker&&) noexcept;

    /**
     * @brief Initializes the focus tracker.
     * @return True if initialization succeeded, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Shuts down the focus tracker.
     */
    void Shutdown();

    /**
     * @brief Gets the currently focused window.
     * @return Handle to the currently focused window, or nullptr if none.
     */
    HWND GetFocusedWindow() const;

    /**
     * @brief Gets the title of the currently focused window.
     * @return Title of the currently focused window, or empty string if none.
     */
    std::wstring GetFocusedWindowTitle() const;

    /**
     * @brief Gets the process ID of the currently focused window.
     * @return Process ID of the currently focused window, or 0 if none.
     */
    DWORD GetFocusedWindowProcessId() const;

    /**
     * @brief Checks if a specified window has focus.
     * @param windowHandle The window handle to check.
     * @return True if the window has focus, false otherwise.
     */
    bool HasFocus(HWND windowHandle) const;

    /**
     * @brief Checks if any window from a specified process has focus.
     * @param processId The process ID to check.
     * @return True if any window from the process has focus, false otherwise.
     */
    bool HasProcessFocus(DWORD processId) const;

    /**
     * @brief Update the focus state.
     * This should be called periodically to check for focus changes.
     */
    void Update();

    /**
     * @brief Registers a callback for focus change events.
     * @param callback The callback function to register.
     * @return ID of the callback for later removal.
     */
    size_t RegisterFocusCallback(const FocusChangeCallback& callback);

    /**
     * @brief Unregisters a focus change callback.
     * @param callbackId ID of the callback to unregister.
     * @return True if the callback was found and removed, false otherwise.
     */
    bool UnregisterFocusCallback(size_t callbackId);

private:
    /**
     * @brief Internal structure for storing callbacks with their IDs.
     */
    struct CallbackEntry {
        size_t id;
        FocusChangeCallback callback;
    };

    /**
     * @brief Updates the focus change information.
     * @param currentWindow The currently focused window.
     * @return True if focus changed, false otherwise.
     */
    bool UpdateFocusInfo(HWND currentWindow);

    /**
     * @brief Gets information about a window.
     * @param windowHandle The window handle to get info for.
     * @param title Output parameter for the window title.
     * @param processId Output parameter for the window process ID.
     * @return True if information was retrieved successfully, false otherwise.
     */
    bool GetWindowInfo(HWND windowHandle, std::wstring& title, DWORD& processId) const;

    /**
     * @brief Notifies all registered callbacks about a focus change event.
     * @param info The focus change information.
     */
    void NotifyFocusChange(const FocusChangeInfo& info);

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
    
    FocusChangeInfo m_lastFocusInfo;               ///< Information about the last focus change
    mutable std::mutex m_focusInfoMutex;           ///< Mutex for thread-safe access to focus info
    
    std::vector<CallbackEntry> m_callbacks;        ///< Registered focus callbacks
    std::mutex m_callbacksMutex;                   ///< Mutex for thread-safe access to callbacks
    size_t m_nextCallbackId;                       ///< Counter for generating callback IDs
};

} // namespace poe