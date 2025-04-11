#include "process/focus_tracker.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"

#include <iostream>

namespace poe {

FocusTracker::FocusTracker(Application& app)
    : m_app(app)
    , m_initialized(false)
    , m_nextCallbackId(1)
{
    // Initialize last focus info
    m_lastFocusInfo.previousWindow = nullptr;
    m_lastFocusInfo.currentWindow = nullptr;
    m_lastFocusInfo.previousProcessId = 0;
    m_lastFocusInfo.currentProcessId = 0;
    m_lastFocusInfo.timestamp = std::chrono::steady_clock::now();
}

FocusTracker::~FocusTracker()
{
    Shutdown();
}

FocusTracker::FocusTracker(FocusTracker&& other) noexcept
    : m_app(other.m_app)
    , m_initialized(other.m_initialized)
    , m_nextCallbackId(other.m_nextCallbackId)
{
    // Move focus info
    std::lock_guard<std::mutex> focusLock(other.m_focusInfoMutex);
    m_lastFocusInfo = other.m_lastFocusInfo;
    
    // Move callbacks
    std::lock_guard<std::mutex> callbackLock(other.m_callbacksMutex);
    m_callbacks = std::move(other.m_callbacks);
    
    // Reset the moved-from object
    other.m_initialized = false;
    other.m_nextCallbackId = 1;
}

FocusTracker& FocusTracker::operator=(FocusTracker&& other) noexcept
{
    if (this != &other)
    {
        // Shutdown current instance
        Shutdown();
        
        // Move members
        m_initialized = other.m_initialized;
        m_nextCallbackId = other.m_nextCallbackId;
        
        // Move focus info
        {
            std::lock_guard<std::mutex> focusLock(other.m_focusInfoMutex);
            m_lastFocusInfo = other.m_lastFocusInfo;
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

bool FocusTracker::Initialize()
{
    if (m_initialized)
    {
        return true;
    }
    
    try
    {
        Log(2, "Initializing FocusTracker");
        
        // Get initial focus state
        HWND currentFocus = GetForegroundWindow();
        UpdateFocusInfo(currentFocus);
        
        m_initialized = true;
        Log(2, "FocusTracker initialized successfully");
        return true;
    }
    catch (const std::exception& ex)
    {
        m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "FocusTracker");
        return false;
    }
}

void FocusTracker::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }
    
    Log(2, "Shutting down FocusTracker");
    
    // Clear callbacks
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_callbacks.clear();
    }
    
    m_initialized = false;
    Log(2, "FocusTracker shutdown complete");
}

HWND FocusTracker::GetFocusedWindow() const
{
    std::lock_guard<std::mutex> lock(m_focusInfoMutex);
    return m_lastFocusInfo.currentWindow;
}

std::wstring FocusTracker::GetFocusedWindowTitle() const
{
    std::lock_guard<std::mutex> lock(m_focusInfoMutex);
    return m_lastFocusInfo.currentTitle;
}

DWORD FocusTracker::GetFocusedWindowProcessId() const
{
    std::lock_guard<std::mutex> lock(m_focusInfoMutex);
    return m_lastFocusInfo.currentProcessId;
}

bool FocusTracker::HasFocus(HWND windowHandle) const
{
    if (!windowHandle)
    {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_focusInfoMutex);
    return m_lastFocusInfo.currentWindow == windowHandle;
}

bool FocusTracker::HasProcessFocus(DWORD processId) const
{
    if (processId == 0)
    {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_focusInfoMutex);
    return m_lastFocusInfo.currentProcessId == processId;
}

void FocusTracker::Update()
{
    if (!m_initialized)
    {
        return;
    }
    
    HWND currentFocus = GetForegroundWindow();
    UpdateFocusInfo(currentFocus);
}

size_t FocusTracker::RegisterFocusCallback(const FocusChangeCallback& callback)
{
    std::lock_guard<std::mutex> lock(m_callbacksMutex);
    
    size_t callbackId = m_nextCallbackId++;
    m_callbacks.push_back({callbackId, callback});
    
    Log(1, "Registered focus change callback (ID: {})", callbackId);
    return callbackId;
}

bool FocusTracker::UnregisterFocusCallback(size_t callbackId)
{
    std::lock_guard<std::mutex> lock(m_callbacksMutex);
    
    auto it = std::find_if(m_callbacks.begin(), m_callbacks.end(),
        [callbackId](const CallbackEntry& entry) {
            return entry.id == callbackId;
        });
    
    if (it != m_callbacks.end())
    {
        m_callbacks.erase(it);
        Log(1, "Unregistered focus change callback (ID: {})", callbackId);
        return true;
    }
    
    return false;
}

bool FocusTracker::UpdateFocusInfo(HWND currentWindow)
{
    if (!m_initialized)
    {
        return false;
    }
    
    bool focusChanged = false;
    
    // Create new focus info
    FocusChangeInfo newInfo;
    newInfo.timestamp = std::chrono::steady_clock::now();
    
    {
        std::lock_guard<std::mutex> lock(m_focusInfoMutex);
        
        // Check if focus has changed
        if (m_lastFocusInfo.currentWindow != currentWindow)
        {
            // Focus has changed, update info
            newInfo.previousWindow = m_lastFocusInfo.currentWindow;
            newInfo.previousTitle = m_lastFocusInfo.currentTitle;
            newInfo.previousProcessId = m_lastFocusInfo.currentProcessId;
            
            newInfo.currentWindow = currentWindow;
            GetWindowInfo(currentWindow, newInfo.currentTitle, newInfo.currentProcessId);
            
            // Update last focus info
            m_lastFocusInfo = newInfo;
            
            focusChanged = true;
        }
    }
    
    // Notify callbacks if focus changed
    if (focusChanged)
    {
        NotifyFocusChange(newInfo);
        
        Log(2, "Focus changed from '{}' to '{}'",
            std::string(newInfo.previousTitle.begin(), newInfo.previousTitle.end()),
            std::string(newInfo.currentTitle.begin(), newInfo.currentTitle.end()));
    }
    
    return focusChanged;
}

bool FocusTracker::GetWindowInfo(HWND windowHandle, std::wstring& title, DWORD& processId) const
{
    if (!windowHandle || !IsWindow(windowHandle))
    {
        title.clear();
        processId = 0;
        return false;
    }
    
    // Get window title
    wchar_t titleBuffer[256] = { 0 };
    GetWindowTextW(windowHandle, titleBuffer, sizeof(titleBuffer) / sizeof(titleBuffer[0]));
    title = titleBuffer;
    
    // Get process ID
    GetWindowThreadProcessId(windowHandle, &processId);
    
    return true;
}

void FocusTracker::NotifyFocusChange(const FocusChangeInfo& info)
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
            entry.callback(info);
        }
        catch (const std::exception& ex)
        {
            m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "FocusTracker");
        }
    }
}

template<typename... Args>
void FocusTracker::Log(int level, const std::string& fmt, const Args&... args)
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
        std::cerr << "FocusTracker log error: " << e.what() << std::endl;
    }
}

} // namespace poe