#include "process/process_detector.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"

#include <TlHelp32.h>
#include <psapi.h>
#include <iostream>

namespace poe {

ProcessDetector::ProcessDetector(Application& app)
    : m_app(app)
    , m_initialized(false)
    , m_running(false)
    , m_nextCallbackId(1)
    , m_monitorInterval(500) // 500ms default interval
{
    // Initialize target process info with default values
    m_targetProcessInfo.processId = 0;
    m_targetProcessInfo.windowHandle = nullptr;
    m_targetProcessInfo.state = ProcessState::NotFound;
    m_targetProcessInfo.hasFocus = false;
    m_targetProcessInfo.isMinimized = false;
    m_targetProcessInfo.windowRect = {0, 0, 0, 0};
}

ProcessDetector::~ProcessDetector()
{
    Shutdown();
}

ProcessDetector::ProcessDetector(ProcessDetector&& other) noexcept
    : m_app(other.m_app)
    , m_initialized(other.m_initialized)
    , m_running(other.m_running.load())
    , m_targetProcessName(std::move(other.m_targetProcessName))
    , m_targetWindowTitle(std::move(other.m_targetWindowTitle))
    , m_targetProcessInfo(other.m_targetProcessInfo)
    , m_nextCallbackId(other.m_nextCallbackId)
    , m_monitorInterval(other.m_monitorInterval)
{
    // Move callbacks
    std::lock_guard<std::mutex> lock(other.m_callbacksMutex);
    m_callbacks = std::move(other.m_callbacks);
    
    // Move monitor thread
    m_monitorThread = std::move(other.m_monitorThread);
    
    // Reset the moved-from object
    other.m_initialized = false;
    other.m_running = false;
    other.m_nextCallbackId = 1;
}

ProcessDetector& ProcessDetector::operator=(ProcessDetector&& other) noexcept
{
    if (this != &other)
    {
        // Shutdown current instance
        Shutdown();
        
        // Move members
        m_initialized = other.m_initialized;
        m_running = other.m_running.load();
        m_targetProcessName = std::move(other.m_targetProcessName);
        m_targetWindowTitle = std::move(other.m_targetWindowTitle);
        m_targetProcessInfo = other.m_targetProcessInfo;
        m_nextCallbackId = other.m_nextCallbackId;
        m_monitorInterval = other.m_monitorInterval;
        
        // Move callbacks
        {
            std::lock_guard<std::mutex> lock(other.m_callbacksMutex);
            m_callbacks = std::move(other.m_callbacks);
        }
        
        // Move monitor thread
        m_monitorThread = std::move(other.m_monitorThread);
        
        // Reset the moved-from object
        other.m_initialized = false;
        other.m_running = false;
        other.m_nextCallbackId = 1;
    }
    
    return *this;
}

bool ProcessDetector::Initialize()
{
    if (m_initialized)
    {
        return true;
    }
    
    try
    {
        Log(2, "Initializing ProcessDetector");
        
        // Start monitoring thread if not already running
        if (!m_running)
        {
            m_running = true;
            m_monitorThread = std::make_unique<std::thread>(&ProcessDetector::MonitorThread, this);
        }
        
        m_initialized = true;
        Log(2, "ProcessDetector initialized successfully");
        return true;
    }
    catch (const std::exception& ex)
    {
        m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "ProcessDetector");
        return false;
    }
}

void ProcessDetector::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }
    
    Log(2, "Shutting down ProcessDetector");
    
    // Stop monitoring thread
    if (m_running)
    {
        m_running = false;
        
        if (m_monitorThread && m_monitorThread->joinable())
        {
            m_monitorThread->join();
        }
        
        m_monitorThread.reset();
    }
    
    // Clear callbacks
    {
        std::lock_guard<std::mutex> lock(m_callbacksMutex);
        m_callbacks.clear();
    }
    
    m_initialized = false;
    Log(2, "ProcessDetector shutdown complete");
}

ProcessInfo ProcessDetector::FindProcess(const std::wstring& processName, const std::wstring& windowTitle)
{
    return UpdateProcessInfo(processName, windowTitle);
}

bool ProcessDetector::IsProcessRunning(const std::wstring& processName, const std::wstring& windowTitle)
{
    ProcessInfo info = FindProcess(processName, windowTitle);
    return info.state == ProcessState::Running;
}

HWND ProcessDetector::GetProcessWindowHandle(const std::wstring& processName, const std::wstring& windowTitle)
{
    ProcessInfo info = FindProcess(processName, windowTitle);
    return info.windowHandle;
}

bool ProcessDetector::HasWindowFocus(HWND windowHandle)
{
    if (!windowHandle || !IsWindow(windowHandle))
    {
        return false;
    }
    
    return GetForegroundWindow() == windowHandle;
}

size_t ProcessDetector::RegisterStateCallback(const ProcessStateCallback& callback)
{
    std::lock_guard<std::mutex> lock(m_callbacksMutex);
    
    size_t callbackId = m_nextCallbackId++;
    m_callbacks.push_back({callbackId, callback});
    
    Log(1, "Registered process state callback (ID: {})", callbackId);
    return callbackId;
}

bool ProcessDetector::UnregisterStateCallback(size_t callbackId)
{
    std::lock_guard<std::mutex> lock(m_callbacksMutex);
    
    auto it = std::find_if(m_callbacks.begin(), m_callbacks.end(),
        [callbackId](const CallbackEntry& entry) {
            return entry.id == callbackId;
        });
    
    if (it != m_callbacks.end())
    {
        m_callbacks.erase(it);
        Log(1, "Unregistered process state callback (ID: {})", callbackId);
        return true;
    }
    
    return false;
}

void ProcessDetector::Update()
{
    if (!m_initialized)
    {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_processMutex);
    
    // Update target process info
    ProcessInfo newInfo = UpdateProcessInfo(m_targetProcessName, m_targetWindowTitle);
    
    // Check for state changes
    if (newInfo.state != m_targetProcessInfo.state ||
        newInfo.hasFocus != m_targetProcessInfo.hasFocus ||
        newInfo.isMinimized != m_targetProcessInfo.isMinimized)
    {
        // Update stored info
        m_targetProcessInfo = newInfo;
        
        // Notify callbacks about state change
        NotifyStateChange(m_targetProcessInfo);
    }
    else
    {
        // Just update the info without notification
        m_targetProcessInfo = newInfo;
    }
}

void ProcessDetector::SetTargetProcess(const std::wstring& processName, const std::wstring& windowTitle)
{
    std::lock_guard<std::mutex> lock(m_processMutex);
    
    m_targetProcessName = processName;
    m_targetWindowTitle = windowTitle;
    
    // Update info immediately
    m_targetProcessInfo = UpdateProcessInfo(processName, windowTitle);
    
    Log(2, "Set target process: '{}' with window title '{}'",
        std::string(processName.begin(), processName.end()),
        std::string(windowTitle.begin(), windowTitle.end()));
}

const ProcessInfo& ProcessDetector::GetTargetProcessInfo() const
{
    return m_targetProcessInfo;
}

void ProcessDetector::MonitorThread()
{
    Log(2, "Process monitor thread started");
    
    while (m_running)
    {
        // Update process information
        Update();
        
        // Sleep for monitoring interval
        std::this_thread::sleep_for(m_monitorInterval);
    }
    
    Log(2, "Process monitor thread stopped");
}

ProcessInfo ProcessDetector::UpdateProcessInfo(const std::wstring& processName, const std::wstring& windowTitle)
{
    ProcessInfo info;
    info.name = processName;
    info.windowTitle = windowTitle;
    info.processId = 0;
    info.windowHandle = nullptr;
    info.state = ProcessState::NotFound;
    info.hasFocus = false;
    info.isMinimized = false;
    info.windowRect = {0, 0, 0, 0};
    
    // Find window by title (if provided)
    if (!windowTitle.empty())
    {
        info.windowHandle = FindWindowW(nullptr, windowTitle.c_str());
        
        // If exact match not found, try partial match
        if (!info.windowHandle)
        {
            // Use EnumWindows to find window with partial title match
            struct EnumData {
                const std::wstring& targetTitle;
                HWND result;
            };
            
            EnumData data = { windowTitle, nullptr };
            
            EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                auto data = reinterpret_cast<EnumData*>(lParam);
                
                // Get window title
                wchar_t title[256] = { 0 };
                GetWindowTextW(hwnd, title, sizeof(title) / sizeof(title[0]));
                
                // Check if window is visible and title contains the target title
                if (IsWindowVisible(hwnd) && wcsstr(title, data->targetTitle.c_str()) != nullptr)
                {
                    data->result = hwnd;
                    return FALSE; // Stop enumeration
                }
                
                return TRUE; // Continue enumeration
            }, reinterpret_cast<LPARAM>(&data));
            
            info.windowHandle = data.result;
        }
    }
    
    // If we found a window handle
    if (info.windowHandle)
    {
        // Get process ID from window handle
        GetWindowThreadProcessId(info.windowHandle, &info.processId);
        
        // Get window position and state
        info.hasFocus = (GetForegroundWindow() == info.windowHandle);
        info.isMinimized = IsIconic(info.windowHandle);
        GetWindowRect(info.windowHandle, &info.windowRect);
        
        // Get process name if not provided
        if (processName.empty() && info.processId != 0)
        {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, info.processId);
            if (hProcess)
            {
                wchar_t processNameBuffer[MAX_PATH] = { 0 };
                DWORD size = sizeof(processNameBuffer) / sizeof(processNameBuffer[0]);
                
                if (QueryFullProcessImageNameW(hProcess, 0, processNameBuffer, &size))
                {
                    // Extract filename from path
                    wchar_t* fileName = wcsrchr(processNameBuffer, L'\\');
                    if (fileName)
                    {
                        info.name = fileName + 1; // Skip backslash
                    }
                    else
                    {
                        info.name = processNameBuffer;
                    }
                }
                
                CloseHandle(hProcess);
            }
        }
        
        info.state = ProcessState::Running;
    }
    else if (!processName.empty())
    {
        // If window not found but process name provided, check if process is running
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE)
        {
            PROCESSENTRY32W pe32 = { 0 };
            pe32.dwSize = sizeof(PROCESSENTRY32W);
            
            if (Process32FirstW(hSnapshot, &pe32))
            {
                do
                {
                    if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0)
                    {
                        info.processId = pe32.th32ProcessID;
                        info.state = ProcessState::Running;
                        
                        // Try to find the window for this process
                        struct EnumData {
                            DWORD processId;
                            HWND result;
                        };
                        
                        EnumData data = { info.processId, nullptr };
                        
                        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                            auto data = reinterpret_cast<EnumData*>(lParam);
                            
                            DWORD windowProcessId = 0;
                            GetWindowThreadProcessId(hwnd, &windowProcessId);
                            
                            if (windowProcessId == data->processId && IsWindowVisible(hwnd))
                            {
                                data->result = hwnd;
                                return FALSE; // Stop enumeration
                            }
                            
                            return TRUE; // Continue enumeration
                        }, reinterpret_cast<LPARAM>(&data));
                        
                        if (data.result)
                        {
                            info.windowHandle = data.result;
                            
                            // Get window title
                            wchar_t title[256] = { 0 };
                            GetWindowTextW(info.windowHandle, title, sizeof(title) / sizeof(title[0]));
                            info.windowTitle = title;
                            
                            // Get window position and state
                            info.hasFocus = (GetForegroundWindow() == info.windowHandle);
                            info.isMinimized = IsIconic(info.windowHandle);
                            GetWindowRect(info.windowHandle, &info.windowRect);
                        }
                        
                        break;
                    }
                } while (Process32NextW(hSnapshot, &pe32));
            }
            
            CloseHandle(hSnapshot);
        }
    }
    
    return info;
}

void ProcessDetector::NotifyStateChange(const ProcessInfo& info)
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
            m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "ProcessDetector");
        }
    }
}

template<typename... Args>
void ProcessDetector::Log(int level, const std::string& fmt, const Args&... args)
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
        std::cerr << "ProcessDetector log error: " << e.what() << std::endl;
    }
}

} // namespace poe