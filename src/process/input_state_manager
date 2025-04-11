#include "process/input_state_manager.h"
#include "process/process_detector.h"
#include "process/focus_tracker.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"

#include <iostream>

namespace poe {

InputStateManager::InputStateManager(
    Application& app,
    ProcessDetector& processDetector,
    FocusTracker& focusTracker)
    : m_app(app)
    , m_processDetector(processDetector)
    , m_focusTracker(focusTracker)
    , m_initialized(false)
    , m_gameWindowHandle(nullptr)
    , m_overlayWindowHandle(nullptr)
    , m_nextCallbackId(1)
{
    // Initialize current state
    m_currentState.mode = InputMode::Normal;
    m_currentState.keyboardState = InputState::Active;
    m_currentState.mouseState = InputState::Active;
    m_currentState.gameHasFocus = false;
    m_currentState.overlayHasFocus = false;
    m_currentState.mousePosition = {0, 0};
    m_currentState.timestamp = std::chrono::steady_clock::now();
}

InputStateManager::~InputStateManager()
{
    Shutdown();
}

InputStateManager::InputStateManager(InputStateManager&& other) noexcept
    : m_app(other.m_app)
    , m_processDetector(other.m_processDetector)
    , m_focusTracker(other.m_focusTracker)
    , m_initialized(other.m_initialized)
    , m_gameWindowHandle(other.m_gameWindowHandle)
    , m_overlayWindowHandle(other.m_overlayWindowHandle)
    , m_nextCallbackId(other.m_nextCallbackId)
{
    // Move state
    std::lock_guard<std::mutex> stateLock(other.m_stateMutex);
    m_currentState = other.m_currentState;
    
    // Move callbacks
    std::lock_guard<std::mutex> callbackLock(other.m_callbacksMutex);
    m_callbacks = std::move(other.m_callbacks);
    
    // Reset the moved-from object
    other.m_initialized = false;
    other.m_gameWindowHandle = nullptr;
    other.m_overlayWindowHandle = nullptr;
    other.m_nextCallbackId = 1;
}

InputStateManager& InputStateManager::operator=(InputStateManager&& other) noexcept
{
    if (this != &other)
    {
        // Shutdown current instance
        Shutdown();
        
        // Move members
        m_initialized = other.m_initialized;
        m_gameWindowHandle = other.m_gameWindowHandle;
        m_overlayWindowHandle = other.m_overlayWindowHandle;
        m_nextCallbackId = other.m_nextCallbackId;
        
        // Move state
        {
            std::lock_guard<std::mutex> stateLock(other.m_stateMutex);
            m_currentState = other.m_currentState;
        }
        
        // Move callbacks
        {
            std::lock_guard<std::mutex> callbackLock(other.m_callbacksMutex);
            m_callbacks = std::move(other.m_callbacks);
        }
        
        // Reset the moved-from object
        other.m_initialized = false;
        other.m_gameWindowHandle = nullptr;
        other.m_overlayWindowHandle = nullptr;
        other.m_nextCallbackId = 1;
    }
    
    return *this;
}

bool InputStateManager::Initialize()
{
    if (m_initialized)
    {
        return true;
    }
    
    try
    {
        Log(2, "Initializing InputStateManager");
        
        // Initialize current state
        UpdateInputState();
        
        m_initialized = true;
        Log(2, "InputStateManager initialized successfully");
        return true;
    }
    catch (const std::exception& ex)
    {
        m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "InputStateManager");
        return false;
    }
}

void InputStateManager::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }
    
    Log(2, "Shutting down InputStateManager");
    
    // Clear callbacks
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_callbacks.clear();
    }
    
    m_initialized = false;
    Log(2, "InputStateManager shutdown complete");
}

bool InputStateManager::SetInputMode(InputMode mode)
{
    if (!m_initialized)
    {
        return false;
    }
    
    // Store old state for comparison
    InputStateInfo oldState;
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        oldState = m_currentState;
        
        // Set new mode
        m_currentState.mode = mode;
        m_currentState.timestamp = std::chrono::steady_clock::now();
    }
    
    // Apply the new mode
    ApplyInputMode();
    
    // Get updated state for notification
    InputStateInfo newState;
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        newState = m_currentState;
    }
    
    // Check if state actually changed
    if (oldState.mode != newState.mode ||
        oldState.keyboardState != newState.keyboardState ||
        oldState.mouseState != newState.mouseState)
    {
        // Notify callbacks about state change
        NotifyStateChange(oldState, newState);
        
        Log(2, "Input mode changed to {}", static_cast<int>(mode));
    }
    
    return true;
}

InputStateInfo InputStateManager::GetInputState() const
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_currentState;
}

InputMode InputStateManager::GetInputMode() const
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_currentState.mode;
}

void InputStateManager::SetGameWindow(HWND gameWindowHandle)
{
    if (m_gameWindowHandle != gameWindowHandle)
    {
        m_gameWindowHandle = gameWindowHandle;
        
        // Update state to reflect new game window
        UpdateInputState();
        
        if (gameWindowHandle)
        {
            wchar_t titleBuffer[256] = { 0 };
            GetWindowTextW(gameWindowHandle, titleBuffer, sizeof(titleBuffer) / sizeof(titleBuffer[0]));
            
            Log(2, "Game window set to '{}' (Handle: {:p})",
                std::string(titleBuffer, titleBuffer + wcslen(titleBuffer)),
                gameWindowHandle);
        }
        else
        {
            Log(2, "Game window cleared");
        }
    }
}

void InputStateManager::SetOverlayWindow(HWND overlayWindowHandle)
{
    if (m_overlayWindowHandle != overlayWindowHandle)
    {
        m_overlayWindowHandle = overlayWindowHandle;
        
        // Update state to reflect new overlay window
        UpdateInputState();
        
        if (overlayWindowHandle)
        {
            wchar_t titleBuffer[256] = { 0 };
            GetWindowTextW(overlayWindowHandle, titleBuffer, sizeof(titleBuffer) / sizeof(titleBuffer[0]));
            
            Log(2, "Overlay window set to '{}' (Handle: {:p})",
                std::string(titleBuffer, titleBuffer + wcslen(titleBuffer)),
                overlayWindowHandle);
        }
        else
        {
            Log(2, "Overlay window cleared");
        }
    }
}

void InputStateManager::Update()
{
    if (!m_initialized)
    {
        return;
    }
    
    // Store old state for comparison
    InputStateInfo oldState;
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        oldState = m_currentState;
    }
    
    // Update state based on current focus and window state
    UpdateInputState();
    
    // Apply current input mode
    ApplyInputMode();
    
    // Get updated state for notification
    InputStateInfo newState;
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        newState = m_currentState;
    }
    
    // Check if state actually changed
    if (oldState.gameHasFocus != newState.gameHasFocus ||
        oldState.overlayHasFocus != newState.overlayHasFocus ||
        oldState.keyboardState != newState.keyboardState ||
        oldState.mouseState != newState.mouseState)
    {
        // Notify callbacks about state change
        NotifyStateChange(oldState, newState);
    }
}

size_t InputStateManager::RegisterStateCallback(const InputStateCallback& callback)
{
    std::lock_guard<std::mutex> lock(m_callbacksMutex);
    
    size_t callbackId = m_nextCallbackId++;
    m_callbacks.push_back({callbackId, callback});
    
    Log(1, "Registered input state callback (ID: {})", callbackId);
    return callbackId;
}

bool InputStateManager::UnregisterStateCallback(size_t callbackId)
{
    std::lock_guard<std::mutex> lock(m_callbacksMutex);
    
    auto it = std::find_if(m_callbacks.begin(), m_callbacks.end(),
        [callbackId](const CallbackEntry& entry) {
            return entry.id == callbackId;
        });
    
    if (it != m_callbacks.end())
    {
        m_callbacks.erase(it);
        Log(1, "Unregistered input state callback (ID: {})", callbackId);
        return true;
    }
    
    return false;
}

bool InputStateManager::ShouldBlockInput() const
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    
    // Block input if keyboard or mouse is blocked
    return m_currentState.keyboardState == InputState::Blocked ||
           m_currentState.mouseState == InputState::Blocked;
}

bool InputStateManager::ShouldPassthroughMouse() const
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    
    // Pass through mouse if in passthrough mode or if game has focus
    return m_currentState.mouseState == InputState::Inactive ||
           m_currentState.mode == InputMode::Passthrough ||
           (m_currentState.mode == InputMode::GameFocused && m_currentState.gameHasFocus);
}

bool InputStateManager::ShouldPassthroughKeyboard() const
{
    std::lock_guard<std::mutex> lock(m_stateMutex);
    
    // Pass through keyboard if in passthrough mode or if game has focus
    return m_currentState.keyboardState == InputState::Inactive ||
           m_currentState.mode == InputMode::Passthrough ||
           (m_currentState.mode == InputMode::GameFocused && m_currentState.gameHasFocus);
}

void InputStateManager::UpdateInputState()
{
    if (!m_initialized)
    {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_stateMutex);
    
    // Get current focused window
    HWND focusedWindow = m_focusTracker.GetFocusedWindow();
    
    // Check if game window has focus
    m_currentState.gameHasFocus = (m_gameWindowHandle && focusedWindow == m_gameWindowHandle);
    
    // Check if overlay window has focus
    m_currentState.overlayHasFocus = (m_overlayWindowHandle && focusedWindow == m_overlayWindowHandle);
    
    // Get current mouse position
    GetCursorPos(&m_currentState.mousePosition);
    
    // Update timestamp
    m_currentState.timestamp = std::chrono::steady_clock::now();
}

void InputStateManager::ApplyInputMode()
{
    if (!m_initialized)
    {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_stateMutex);
    
    // Apply input mode settings
    switch (m_currentState.mode)
    {
        case InputMode::Normal:
            // Normal input handling, both keyboard and mouse are active
            m_currentState.keyboardState = InputState::Active;
            m_currentState.mouseState = InputState::Active;
            break;
            
        case InputMode::Passthrough:
            // Pass input through to underlying window
            m_currentState.keyboardState = InputState::Inactive;
            m_currentState.mouseState = InputState::Inactive;
            break;
            
        case InputMode::Blocked:
            // Block all input
            m_currentState.keyboardState = InputState::Blocked;
            m_currentState.mouseState = InputState::Blocked;
            break;
            
        case InputMode::GameFocused:
            // If game has focus, pass input to game, otherwise handle normally
            if (m_currentState.gameHasFocus)
            {
                m_currentState.keyboardState = InputState::Inactive;
                m_currentState.mouseState = InputState::Inactive;
            }
            else
            {
                m_currentState.keyboardState = InputState::Active;
                m_currentState.mouseState = InputState::Active;
            }
            break;
            
        case InputMode::OverlayFocused:
            // If overlay has focus, handle input normally, otherwise pass through
            if (m_currentState.overlayHasFocus)
            {
                m_currentState.keyboardState = InputState::Active;
                m_currentState.mouseState = InputState::Active;
            }
            else
            {
                m_currentState.keyboardState = InputState::Inactive;
                m_currentState.mouseState = InputState::Inactive;
            }
            break;
    }
}

void InputStateManager::NotifyStateChange(const InputStateInfo& oldState, const InputStateInfo& newState)
{
    // Copy callbacks to avoid holding lock during callbacks
    std::vector<CallbackEntry> callbacks;
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        callbacks = m_callbacks;
    }
    
    // Call each callback
    for (const auto& entry : callbacks)
    {
        try
        {
            entry.callback(oldState, newState);
        }
        catch (const std::exception& ex)
        {
            m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "InputStateManager");
        }
    }
}

template<typename... Args>
void InputStateManager::Log(int level, const std::string& fmt, const Args&... args)
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
        std::cerr << "InputStateManager log error: " << e.what() << std::endl;
    }
}

} // namespace poe