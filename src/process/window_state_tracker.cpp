#include "process/window_state_tracker.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"

#include <iostream>

namespace poe {

WindowStateTracker::WindowStateTracker(Application& app)
    : m_app(app)
    , m_initialized(false)
    , m_nextCallbackId(1)
{
}

WindowStateTracker::~WindowStateTracker()
{
    Shutdown();
}

WindowStateTracker::WindowStateTracker(WindowStateTracker&& other) noexcept
    : m_app(other.m_app)
    , m_initialized(other.m_initialized)
    , m_nextCallbackId(other.m_nextCallbackId)
{
    // Move window states
    std::lock_guard<std::mutex> windowLock(other.m_windowStatesMutex);
    m_windowStates = std::move(other.m_windowStates);
    
    // Move callbacks
    std::lock_guard<std::mutex> callbackLock(other.m_callbacksMutex);
    m_callbacks = std::move(other.m_callbacks);
    
    // Reset the moved-from object
    other.m_initialized = false;
    other.m_nextCallbackId = 1;
}

WindowStateTracker& WindowStateTracker::operator=(WindowStateTracker&& other) noexcept
{
    if (this != &other)
    {
        // Shutdown current instance
        Shutdown();
        
        // Move members
        m_initialized = other.m_initialized;
        m_nextCallbackId = other.m_nextCallbackId;
        
        // Move window states
        {
            std::lock_guard<std::mutex> windowLock(other.m_windowStatesMutex);
            m_windowStates = std::move(other.m_windowStates);
        }
        
        // Move callbacks
        {
            std::lock_guard<std::mutex> callbackLock(other.m_callbacksMutex);
            m_callbacks = std::move(other.m_callbacks);
        }
        
        // Reset the moved-from object
        other.m_initialized = false;
        other.m_nextCallbackId = 1;
    }
    
    return *this;
}

bool WindowStateTracker::Initialize()
{
    if (m_initialized)
    {
        return true;
    }
    
    try
    {
        Log(2, "Initializing WindowStateTracker");
        
        m_initialized = true;
        Log(2, "WindowStateTracker initialized successfully");
        return true;
    }
    catch (const std::exception& ex)
    {
        m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "WindowStateTracker");
        return false;
    }
}

void WindowStateTracker::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }
    
    Log(2, "Shutting down WindowStateTracker");
    
    // Clear tracked windows
    {
        std::lock_guard<std::mutex> lock(m_windowStatesMutex);
        m_windowStates.clear();
    }
    
    // Clear callbacks
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_callbacks.clear();
    }
    
    m_initialized = false;
    Log(2, "WindowStateTracker shutdown complete");
}

bool WindowStateTracker::AddWindow(HWND handle)
{
    if (!m_initialized || !handle || !IsWindow(handle))
    {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_windowStatesMutex);
    
    // Check if window is already tracked
    if (m_windowStates.find(handle) != m_windowStates.end())
    {
        return false;
    }
    
    // Get initial window state
    WindowStateInfo info = GetWindowInfo(handle);
    
    // Add to tracked windows
    m_windowStates[handle] = info;
    
    wchar_t titleBuffer[256] = { 0 };
    GetWindowTextW(handle, titleBuffer, sizeof(titleBuffer) / sizeof(titleBuffer[0]));
    std::wstring title(titleBuffer);
    
    Log(2, "Added window to tracker: '{}' (Handle: {:p})",
        std::string(title.begin(), title.end()),
        handle);
    
    return true;
}

bool WindowStateTracker::RemoveWindow(HWND handle)
{
    if (!m_initialized)
    {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_windowStatesMutex);
    
    auto it = m_windowStates.find(handle);
    if (it == m_windowStates.end())
    {
        return false;
    }
    
    wchar_t titleBuffer[256] = { 0 };
    GetWindowTextW(handle, titleBuffer, sizeof(titleBuffer) / sizeof(titleBuffer[0]));
    std::wstring title(titleBuffer);
    
    Log(2, "Removed window from tracker: '{}' (Handle: {:p})",
        std::string(title.begin(), title.end()),
        handle);
    
    m_windowStates.erase(it);
    return true;
}

WindowStateInfo WindowStateTracker::GetWindowState(HWND handle) const
{
    if (!m_initialized)
    {
        WindowStateInfo invalidInfo;
        invalidInfo.state = WindowState::Invalid;
        return invalidInfo;
    }
    
    std::lock_guard<std::mutex> lock(m_windowStatesMutex);
    
    auto it = m_windowStates.find(handle);
    if (it == m_windowStates.end())
    {
        // Return default state info with Invalid state
        WindowStateInfo info;
        info.handle = handle;
        info.state = WindowState::Invalid;
        return info;
    }
    
    return it->second;
}

bool WindowStateTracker::IsWindowTracked(HWND handle) const
{
    if (!m_initialized)
    {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_windowStatesMutex);
    return m_windowStates.find(handle) != m_windowStates.end();
}

void WindowStateTracker::Update()
{
    if (!m_initialized)
    {
        return;
    }
    
    // Get copy of tracked windows to avoid holding lock during updates
    std::vector<HWND> windows;
    {
        std::lock_guard<std::mutex> lock(m_windowStatesMutex);
        windows.reserve(m_windowStates.size());
        for (const auto& pair : m_windowStates)
        {
            windows.push_back(pair.first);
        }
    }
    
    // Update each window state
    for (HWND handle : windows)
    {
        UpdateWindowState(handle);
    }
}

size_t WindowStateTracker::RegisterStateCallback(const WindowStateCallback& callback)
{
    std::lock_guard<std::mutex> lock(m_callbacksMutex);
    
    size_t callbackId = m_nextCallbackId++;
    m_callbacks.push_back({callbackId, callback});
    
    Log(1, "Registered window state callback (ID: {})", callbackId);
    return callbackId;
}

bool WindowStateTracker::UnregisterStateCallback(size_t callbackId)
{
    std::lock_guard<std::mutex> lock(m_callbacksMutex);
    
    auto it = std::find_if(m_callbacks.begin(), m_callbacks.end(),
        [callbackId](const CallbackEntry& entry) {
            return entry.id == callbackId;
        });
    
    if (it != m_callbacks.end())
    {
        m_callbacks.erase(it);
        Log(1, "Unregistered window state callback (ID: {})", callbackId);
        return true;
    }
    
    return false;
}

bool WindowStateTracker::UpdateWindowState(HWND handle)
{
    if (!m_initialized || !handle)
    {
        return false;
    }
    
    // Get current window info
    WindowStateInfo newInfo = GetWindowInfo(handle);
    
    // Compare with previous state
    WindowStateInfo oldInfo;
    bool stateChanged = false;
    
    {
        std::lock_guard<std::mutex> lock(m_windowStatesMutex);
        
        auto it = m_windowStates.find(handle);
        if (it == m_windowStates.end())
        {
            // Window not tracked
            return false;
        }
        
        oldInfo = it->second;
        
        // Check if state changed
        stateChanged = 
            oldInfo.state != newInfo.state ||
            oldInfo.hasFocus != newInfo.hasFocus ||
            !EqualRect(&oldInfo.bounds, &newInfo.bounds) ||
            oldInfo.isTopmost != newInfo.isTopmost;
        
        if (stateChanged)
        {
            // Update stored state
            it->second = newInfo;
        }
    }
    
    // Notify callbacks if state changed
    if (stateChanged)
    {
        NotifyStateChange(oldInfo, newInfo);
    }
    
    return true;
}

WindowStateInfo WindowStateTracker::GetWindowInfo(HWND handle) const
{
    WindowStateInfo info;
    info.handle = handle;
    info.state = WindowState::Invalid;
    
    if (!handle || !IsWindow(handle))
    {
        return info;
    }
    
    // Get window title
    wchar_t titleBuffer[256] = { 0 };
    GetWindowTextW(handle, titleBuffer, sizeof(titleBuffer) / sizeof(titleBuffer[0]));
    info.title = titleBuffer;
    
    // Get window process ID
    GetWindowThreadProcessId(handle, &info.processId);
    
    // Get window bounds
    GetWindowRect(handle, &info.bounds);
    
    // Get window state
    WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
    GetWindowPlacement(handle, &placement);
    
    if (!IsWindowVisible(handle))
    {
        info.state = WindowState::Hidden;
    }
    else if (placement.showCmd == SW_SHOWMINIMIZED)
    {
        info.state = WindowState::Minimized;
    }
    else if (placement.showCmd == SW_SHOWMAXIMIZED)
    {
        info.state = WindowState::Maximized;
    }
    else
    {
        info.state = WindowState::Normal;
    }
    
    // Check if window has focus
    info.hasFocus = (GetForegroundWindow() == handle);
    
    // Check if window is topmost
    LONG exStyle = GetWindowLongW(handle, GWL_EXSTYLE);
    info.isTopmost = (exStyle & WS_EX_TOPMOST) != 0;
    
    return info;
}

void WindowStateTracker::NotifyStateChange(const WindowStateInfo& oldInfo, const WindowStateInfo& newInfo)
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
            entry.callback(oldInfo, newInfo);
        }
        catch (const std::exception& ex)
        {
            m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "WindowStateTracker");
        }
    }
}

template<typename... Args>
void WindowStateTracker::Log(int level, const std::string& fmt, const Args&... args)
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
        std::cerr << "WindowStateTracker log error: " << e.what() << std::endl;
    }
}

} // namespace poe