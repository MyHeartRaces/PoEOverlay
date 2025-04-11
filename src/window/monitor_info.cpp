/**
 * @file monitor_info.cpp
 * @brief Implementation of the MonitorInfo class
 */

 #include "window/monitor_info.h"
 #include <ShellScalingApi.h>
 
 #pragma comment(lib, "Shcore.lib")
 
 namespace poe {
 
 MonitorInfo::MonitorInfo(HMONITOR monitor) : m_monitor(monitor) {
     MONITORINFOEXW monitorInfo = {};
     monitorInfo.cbSize = sizeof(MONITORINFOEXW);
     
     if (GetMonitorInfoW(monitor, &monitorInfo)) {
         m_workArea = monitorInfo.rcWork;
         m_fullArea = monitorInfo.rcMonitor;
         m_isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
     }
 
     // Get monitor DPI
     UINT dpiX = 96, dpiY = 96;
     if (SUCCEEDED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
         m_scaleFactor = static_cast<float>(dpiX) / 96.0f;
     } else {
         m_scaleFactor = 1.0f;
     }
 }
 
 RECT MonitorInfo::GetWorkArea() const {
     return m_workArea;
 }
 
 RECT MonitorInfo::GetFullArea() const {
     return m_fullArea;
 }
 
 bool MonitorInfo::IsPrimary() const {
     return m_isPrimary;
 }
 
 float MonitorInfo::GetScaleFactor() const {
     return m_scaleFactor;
 }
 
 std::vector<MonitorInfo> GetAllMonitors() {
     std::vector<MonitorInfo> monitors;
     
     // Enumerate all monitors
     EnumDisplayMonitors(
         nullptr, nullptr,
         [](HMONITOR hMonitor, HDC, LPRECT, LPARAM lParam) -> BOOL {
             auto* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(lParam);
             monitors->emplace_back(hMonitor);
             return TRUE;
         },
         reinterpret_cast<LPARAM>(&monitors)
     );
     
     return monitors;
 }
 
 } // namespace poe