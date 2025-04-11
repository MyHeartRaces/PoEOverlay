/**
 * @file monitor_info.h
 * @brief Defines the MonitorInfo class for handling multi-monitor configurations
 * 
 * This header provides functionality for working with multiple monitors,
 * including DPI scaling, work areas, and monitor identification.
 */

 #pragma once

 #include <Windows.h>
 #include <vector>
 
 namespace poe {
 
 /**
  * @class MonitorInfo
  * @brief Stores information about a connected monitor
  */
 class MonitorInfo {
 public:
     /**
      * @brief Construct a new Monitor Info object
      * 
      * @param monitor Monitor handle from Windows API
      */
     explicit MonitorInfo(HMONITOR monitor);
 
     /**
      * @brief Get the monitor's work area (excludes taskbar)
      * 
      * @return RECT The monitor's work area
      */
     RECT GetWorkArea() const;
 
     /**
      * @brief Get the monitor's full area
      * 
      * @return RECT The monitor's full area
      */
     RECT GetFullArea() const;
 
     /**
      * @brief Check if this is the primary monitor
      * 
      * @return true If this is the primary monitor
      * @return false Otherwise
      */
     bool IsPrimary() const;
 
     /**
      * @brief Get the monitor's DPI scaling factor
      * 
      * @return float The DPI scaling factor
      */
     float GetScaleFactor() const;
 
 private:
     HMONITOR m_monitor;   ///< Monitor handle
     RECT m_workArea;      ///< Monitor work area
     RECT m_fullArea;      ///< Monitor full area
     bool m_isPrimary;     ///< Whether this is the primary monitor
     float m_scaleFactor;  ///< DPI scaling factor
 };
 
 /**
  * @brief Get information about all connected monitors
  * 
  * @return std::vector<MonitorInfo> Vector of monitor information
  */
 std::vector<MonitorInfo> GetAllMonitors();
 
 } // namespace poe