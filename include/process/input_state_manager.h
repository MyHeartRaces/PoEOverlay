#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include <chrono>
#include <unordered_map>
#include "core/Application.h"

namespace poe {

// Forward declarations
class Application;
class ProcessDetector;
class FocusTracker;

/**
 * @enum InputMode
 * @brief Enumeration of input handling modes.
 */
enum class InputMode {
    Normal,         ///< Normal input handling
    Passthrough,    ///< Pass input through to underlying window
    Blocked,        ///< Block input
    GameFocused,    ///< Game window has focus
    OverlayFocused  ///< Overlay window has focus
};

/**
 * @enum InputState
 * @brief Enumeration of input device states.
 */
enum class InputState {
    Inactive,       ///< Input device is inactive
    Active,         ///< Input device is active
    Blocked         ///< Input device is blocked
};

/**
 * @struct InputStateInfo
 * @brief Information about the current input state.
 */
struct InputStateInfo {
    InputMode mode = InputMode::Normal;            ///< Current input mode
    InputState keyboardState = InputState::Active; ///< Keyboard state
    InputState mouseState = InputState::Active;    ///< Mouse state
    bool gameHasFocus = false;                     ///< Whether game window has focus
    bool overlayHasFocus = false;                  ///< Whether overlay window has focus
    POINT mousePosition = {0, 0};                  ///< Current mouse position
    std::chrono::steady_clock::time_point timestamp; ///< Timestamp of last update
};

/**
 * @class InputStateManager
 * @brief Manages the input state based on focus and window state.
 * 
 * This class is responsible for tracking and managing input states,
 * determining how input should be handled based on which window
 * has focus and the configured input mode.
 */
class InputStateManager {
public:
    /**
     * @brief Type definition for input state change callbacks.
     */
    using InputStateCallback = std::function<void(const InputStateInfo&, const InputStateInfo&)>;

    /**
     * @brief Constructor for the InputStateManager class.
     * @param app Reference to the main application instance.
     * @param processDetector Reference to the process detector.
     * @param focusTracker Reference to the focus tracker.
     */
    InputStateManager(
        Application& app,
        ProcessDetector& processDetector,
        FocusTracker& focusTracker);

    /**
     * @brief Destructor for the InputStateManager class.
     */
    ~InputStateManager();

    // Non-copyable
    InputStateManager(const InputStateManager&) = delete;
    InputStateManager& operator=(const InputStateManager&) = delete;

    // Movable
    InputStateManager(InputStateManager&&) noexcept;
    InputStateManager& operator=(InputStateManager&&) noexcept;

    /**
     * @brief Initializes the input state manager.
     * @return True if initialization succeeded, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Shuts down the input state manager.
     */
    void Shutdown();

    /**
     * @brief Sets the current input mode.
     * @param mode The input mode to set.
     * @return True if the mode was set, false otherwise.
     */
    bool SetInputMode(InputMode mode);

    /**
     * @brief Gets the current input state information.
     * @return Current input state information.
     */
    InputStateInfo GetInputState() const;

    /**
     * @brief Gets the current input mode.
     * @return Current input mode.
     */
    InputMode GetInputMode() const;

    /**
     * @brief Sets the game window handle.
     * @param gameWindowHandle Handle to the game window.
     */
    void SetGameWindow(HWND gameWindowHandle);

    /**
     * @brief Sets the overlay window handle.
     * @param overlayWindowHandle Handle to the overlay window.
     */
    void SetOverlayWindow(HWND overlayWindowHandle);

    /**
     * @brief Updates the input state.
     * This should be called periodically to update the input state.
     */
    void Update();

    /**
     * @brief Registers a callback for input state changes.
     * @param callback The callback function to register.
     * @return ID of the callback for later removal.
     */
    size_t RegisterStateCallback(const InputStateCallback& callback);

    /**
     * @brief Unregisters an input state callback.
     * @param callbackId ID of the callback to unregister.
     * @return True if the callback was found and removed, false otherwise.
     */
    bool UnregisterStateCallback(size_t callbackId);

    /**
     * @brief Checks if input should be blocked based on current state.
     * @return True if input should be blocked, false otherwise.
     */
    bool ShouldBlockInput() const;

    /**
     * @brief Checks if mouse input should be passed through based on current state.
     * @return True if mouse input should be passed through, false otherwise.
     */
    bool ShouldPassthroughMouse() const;

    /**
     * @brief Checks if keyboard input should be passed through based on current state.
     * @return True if keyboard input should be passed through, false otherwise.
     */
    bool ShouldPassthroughKeyboard() const;

private:
    /**
     * @brief Internal structure for storing callbacks with their IDs.
     */
    struct CallbackEntry {
        size_t id;
        InputStateCallback callback;
    };

    /**
     * @brief Updates input state based on focus and window state.
     */
    void UpdateInputState();

    /**
     * @brief Applies input mode settings to the current state.
     */
    void ApplyInputMode();

    /**
     * @brief Notifies all registered callbacks about a state change.
     * @param oldState The previous input state.
     * @param newState The new input state.
     */
    void NotifyStateChange(const InputStateInfo& oldState, const InputStateInfo& newState);

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
    ProcessDetector& m_processDetector;            ///< Reference to the process detector
    FocusTracker& m_focusTracker;                  ///< Reference to the focus tracker
    bool m_initialized;                            ///< Whether the manager is initialized
    
    HWND m_gameWindowHandle;                       ///< Game window handle
    HWND m_overlayWindowHandle;                    ///< Overlay window handle
    
    InputStateInfo m_currentState;                 ///< Current input state
    mutable std::mutex m_stateMutex;               ///< Mutex for thread-safe access to state
    
    std::vector<CallbackEntry> m_callbacks;        ///< Registered state callbacks
    std::mutex m_callbacksMutex;                   ///< Mutex for thread-safe access to callbacks
    size_t m_nextCallbackId;                       ///< Counter for generating callback IDs
};

} // namespace poe