#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include "core/Application.h"

namespace poe {

// Forward declarations
class Application;

/**
 * @enum ProcessState
 * @brief Enumeration of process states.
 */
enum class ProcessState {
    NotFound,    ///< Process is not running
    Running,     ///< Process is running
    Starting,    ///< Process is starting
    Terminating  ///< Process is terminating
};

/**
 * @struct ProcessInfo
 * @brief Information about a detected process.
 */
struct ProcessInfo {
    std::wstring name;            ///< Process name
    std::wstring windowTitle;     ///< Window title
    DWORD processId = 0;          ///< Process ID
    HWND windowHandle = nullptr;  ///< Window handle
    ProcessState state = ProcessState::NotFound; ///< Current state
    bool hasFocus = false;        ///< Whether the window has focus
    bool isMinimized = false;     ///< Whether the window is minimized
    RECT windowRect = {0};        ///< Window rectangle
};

/**
 * @class ProcessDetector
 * @brief Detects and monitors game processes.
 * 
 * This class is responsible for detecting the game process,
 * monitoring its state, and tracking window focus changes.
 */
class ProcessDetector {
public:
    /**
     * @brief Type definition for process state change callbacks.
     */
    using ProcessStateCallback = std::function<void(const ProcessInfo&)>;

    /**
     * @brief Constructor for the ProcessDetector class.
     * @param app Reference to the main application instance.
     */
    explicit ProcessDetector(Application& app);

    /**
     * @brief Destructor for the ProcessDetector class.
     */
    ~ProcessDetector();

    // Non-copyable
    ProcessDetector(const ProcessDetector&) = delete;
    ProcessDetector& operator=(const ProcessDetector&) = delete;

    // Movable
    ProcessDetector(ProcessDetector&&) noexcept;
    ProcessDetector& operator=(ProcessDetector&&) noexcept;

    /**
     * @brief Initializes the process detector.
     * @return True if initialization succeeded, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Shuts down the process detector.
     */
    void Shutdown();

    /**
     * @brief Finds a process by name or window title.
     * @param processName Name of the process to find (can be empty).
     * @param windowTitle Title or partial title of the window to find (can be empty).
     * @return Process information, with state NotFound if not found.
     */
    ProcessInfo FindProcess(const std::wstring& processName, const std::wstring& windowTitle);

    /**
     * @brief Checks if a specific process is running.
     * @param processName Name of the process to check.
     * @param windowTitle Title or partial title of the window to check.
     * @return True if the process is running, false otherwise.
     */
    bool IsProcessRunning(const std::wstring& processName, const std::wstring& windowTitle);

    /**
     * @brief Gets the handle to a process window.
     * @param processName Name of the process (can be empty).
     * @param windowTitle Title or partial title of the window (can be empty).
     * @return Window handle, or nullptr if not found.
     */
    HWND GetProcessWindowHandle(const std::wstring& processName, const std::wstring& windowTitle);

    /**
     * @brief Checks if a window has focus.
     * @param windowHandle Handle to the window to check.
     * @return True if the window has focus, false otherwise.
     */
    bool HasWindowFocus(HWND windowHandle);

    /**
     * @brief Registers a callback for process state changes.
     * @param callback The callback function to register.
     * @return ID of the callback for later removal.
     */
    size_t RegisterStateCallback(const ProcessStateCallback& callback);

    /**
     * @brief Unregisters a process state callback.
     * @param callbackId ID of the callback to unregister.
     * @return True if the callback was found and removed, false otherwise.
     */
    bool UnregisterStateCallback(size_t callbackId);

    /**
     * @brief Updates the process state.
     * This should be called periodically to check for state changes.
     */
    void Update();

    /**
     * @brief Sets the target process to monitor.
     * @param processName Name of the process to monitor (can be empty).
     * @param windowTitle Title or partial title of the window to monitor (can be empty).
     */
    void SetTargetProcess(const std::wstring& processName, const std::wstring& windowTitle);

    /**
     * @brief Gets information about the current target process.
     * @return Process information for the target process.
     */
    const ProcessInfo& GetTargetProcessInfo() const;

private:
    /**
     * @brief Internal structure for storing callbacks with their IDs.
     */
    struct CallbackEntry {
        size_t id;
        ProcessStateCallback callback;
    };

    /**
     * @brief Background thread function for monitoring processes.
     */
    void MonitorThread();

    /**
     * @brief Updates the internal process info.
     * @param processName Name of the process to update info for.
     * @param windowTitle Title or partial title of the window to update info for.
     * @return Updated process information.
     */
    ProcessInfo UpdateProcessInfo(const std::wstring& processName, const std::wstring& windowTitle);

    /**
     * @brief Notifies all registered callbacks about a state change.
     * @param info The process information to notify about.
     */
    void NotifyStateChange(const ProcessInfo& info);

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
    bool m_initialized;                            ///< Whether the detector is initialized
    std::atomic<bool> m_running;                   ///< Whether the monitoring thread is running
    
    std::wstring m_targetProcessName;              ///< Name of the target process
    std::wstring m_targetWindowTitle;              ///< Title of the target window
    ProcessInfo m_targetProcessInfo;               ///< Information about the target process
    
    std::vector<CallbackEntry> m_callbacks;        ///< Registered state callbacks
    std::mutex m_callbacksMutex;                   ///< Mutex for thread-safe access to callbacks
    size_t m_nextCallbackId;                       ///< Counter for generating callback IDs
    
    std::unique_ptr<std::thread> m_monitorThread;  ///< Background thread for monitoring
    std::chrono::milliseconds m_monitorInterval;   ///< Interval for monitoring checks
    std::mutex m_processMutex;                     ///< Mutex for thread-safe access to process info
};

} // namespace poe