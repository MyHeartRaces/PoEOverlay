/**
 * @file window_manager.h
 * @brief Defines the WindowManager class for managing overlay windows
 * 
 * This header provides the WindowManager class that coordinates between
 * the overlay window and the game window, handling positioning, input,
 * and window state changes.
 */

 #pragma once

 #include <Windows.h>
 #include <string>
 #include <memory>
 #include <functional>
 #include "window/overlay_window.h"
 #include "core/Application.h"
 
 namespace poe {
 
 // Forward declarations
 class Application;
 
 /**
  * @class WindowManager
  * @brief Manages the overlay window and its relationship with the game window
  */
 class WindowManager {
 public:
     /**
      * @struct WindowManagerConfig
      * @brief Configuration parameters for the window manager
      */
     struct WindowManagerConfig {
         std::wstring gameWindowClassName;  ///< Class name of the game window
         std::wstring gameWindowTitle;      ///< Title (or partial title) of the game window
         bool autoAttachToGame = true;      ///< Whether to automatically attach to game when found
         bool followGameWindow = true;      ///< Whether to follow the game window when it moves
         bool exitWhenGameCloses = true;    ///< Whether to exit when the game window closes
     };
 
     /**
      * @brief Construct a new Window Manager object
      * 
      * @param app Reference to the main application
      * @param config Window manager configuration
      * @param windowConfig Configuration for the overlay window
      */
     WindowManager(
         Application& app,
         const WindowManagerConfig& config = {},
         const OverlayWindow::WindowConfig& windowConfig = {}
     );
 
     /**
      * @brief Destroy the Window Manager object
      */
     ~WindowManager();
 
     /**
      * @brief Initialize the window manager and create the overlay window
      * 
      * @return true If initialization succeeded
      * @return false If initialization failed
      */
     bool Initialize();
 
     /**
      * @brief Run the main message loop
      * 
      * @return int Exit code
      */
     int Run();
 
     /**
      * @brief Find the game window based on configured class name and title
      * 
      * @return HWND Handle to the game window, or nullptr if not found
      */
     HWND FindGameWindow() const;
 
     /**
      * @brief Attach the overlay to the game window
      * 
      * @param gameWindowHandle Handle to the game window
      * @return true If attachment succeeded
      * @return false If attachment failed
      */
     bool AttachToGame(HWND gameWindowHandle);
 
     /**
      * @brief Detach the overlay from the game window
      */
     void DetachFromGame();
 
     /**
      * @brief Check if the overlay is currently attached to a game window
      * 
      * @return true If attached
      * @return false If not attached
      */
     bool IsAttachedToGame() const;
 
     /**
      * @brief Get the game window handle
      * 
      * @return HWND Game window handle or nullptr if not attached
      */
     HWND GetGameWindowHandle() const;
 
     /**
      * @brief Get the overlay window
      * 
      * @return OverlayWindow& Reference to the overlay window
      */
     OverlayWindow& GetOverlayWindow();
 
     /**
      * @brief Set the overlay interaction mode
      * 
      * @param mode The desired interaction mode
      */
     void SetInteractionMode(OverlayWindow::WindowMode mode);
 
     /**
      * @brief Toggle the overlay interaction mode
      */
     void ToggleInteractionMode();
 
     /**
      * @brief Toggle the overlay visibility
      */
     void ToggleVisibility();
 
 private:
     /**
      * @brief Set up the window event callbacks
      */
     void SetupEventCallbacks();
 
     /**
      * @brief Check if the game window still exists and is valid
      * 
      * @return true If game window is valid
      * @return false If game window is invalid or closed
      */
     bool IsGameWindowValid() const;
 
     /**
      * @brief Update the overlay position to match the game window
      */
     void UpdateOverlayPosition();
 
     /**
      * @brief Handle window events from the overlay window
      */
     LRESULT HandleWindowEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
 
     Application& m_app;                           ///< Reference to the main application
     WindowManagerConfig m_config;                 ///< Window manager configuration
     OverlayWindow m_overlayWindow;                ///< The overlay window
     HWND m_gameWindowHandle = nullptr;            ///< Handle to the attached game window
     bool m_running = false;                       ///< Whether the main loop is running
 };
 
 } // namespace poe