/**
 * @file window_manager.cpp
 * @brief Implementation of the WindowManager class
 */

 #include "window/window_manager.h"
 #include <algorithm>
 #include <chrono>
 #include <thread>
 #include "core/Logger.h"
 #include "core/ErrorHandler.h"
 #include "core/EventSystem.h"
 
 namespace poe {
 
 WindowManager::WindowManager(
     Application& app,
     const WindowManagerConfig& config,
     const OverlayWindow::WindowConfig& windowConfig)
     : m_app(app),
       m_config(config),
       m_overlayWindow(app, windowConfig) {
     
     m_app.GetLogger().Info("WindowManager created");
 }
 
 WindowManager::~WindowManager() {
     m_running = false;
     DetachFromGame();
 }
 
 bool WindowManager::Initialize() {
     try {
         m_app.GetLogger().Debug("Initializing WindowManager");
         
         // Create the overlay window
         if (!m_overlayWindow.Create()) {
             m_app.GetErrorHandler().ReportError(
                 ErrorSeverity::Error, 
                 "Failed to create overlay window",
                 "WindowManager"
             );
             return false;
         }
 
         // Set up event callbacks
         SetupEventCallbacks();
 
         // Try to find and attach to the game window if auto-attach is enabled
         if (m_config.autoAttachToGame) {
             HWND gameWindow = FindGameWindow();
             if (gameWindow) {
                 AttachToGame(gameWindow);
             } else {
                 m_app.GetLogger().Info("Game window not found, will keep searching");
             }
         }
 
         m_app.GetLogger().Info("WindowManager initialized successfully");
         return true;
     }
     catch (const std::exception& ex) {
         m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "WindowManager");
         return false;
     }
 }
 
 int WindowManager::Run() {
     m_running = true;
 
     // Main message loop
     while (m_running) {
         // Process window messages
         if (!m_overlayWindow.ProcessMessages()) {
             break;  // Exit loop if ProcessMessages returns false (WM_QUIT)
         }
 
         // Check game window status and update overlay position if needed
         if (m_gameWindowHandle) {
             if (!IsGameWindowValid()) {
                 // Game window was closed or is no longer valid
                 if (m_config.exitWhenGameCloses) {
                     break;  // Exit the application
                 } else {
                     DetachFromGame();  // Just detach from the game
                 }
             } else if (m_config.followGameWindow) {
                 UpdateOverlayPosition();
             }
         } else if (m_config.autoAttachToGame) {
             // Try to find the game window periodically
             static auto lastCheck = std::chrono::steady_clock::now();
             auto now = std::chrono::steady_clock::now();
             
             if (now - lastCheck > std::chrono::seconds(1)) {
                 HWND gameWindow = FindGameWindow();
                 if (gameWindow) {
                     AttachToGame(gameWindow);
                 }
                 lastCheck = now;
             }
         }
 
         // Yield to other threads to avoid high CPU usage
         std::this_thread::sleep_for(std::chrono::milliseconds(10));
     }
 
     return 0;
 }
 
 HWND WindowManager::FindGameWindow() const {
     // Find window by class name and/or title
     return FindWindowW(
         m_config.gameWindowClassName.empty() ? nullptr : m_config.gameWindowClassName.c_str(),
         m_config.gameWindowTitle.empty() ? nullptr : m_config.gameWindowTitle.c_str()
     );
 }
 
 bool WindowManager::AttachToGame(HWND gameWindowHandle) {
     if (!IsWindow(gameWindowHandle)) {
         m_app.GetLogger().Warning("Attempted to attach to invalid game window");
         return false;
     }
 
     m_gameWindowHandle = gameWindowHandle;
     
     // Get window title for logging
     wchar_t windowTitle[256] = {0};
     GetWindowTextW(gameWindowHandle, windowTitle, sizeof(windowTitle)/sizeof(windowTitle[0]));
     
     m_app.GetLogger().Info("Attached to game window: '{}'", 
                            std::string(windowTitle, windowTitle + wcslen(windowTitle)));
     
     // Align the overlay with the game window
     UpdateOverlayPosition();
     
     return true;
 }
 
 void WindowManager::DetachFromGame() {
     m_gameWindowHandle = nullptr;
 }
 
 bool WindowManager::IsAttachedToGame() const {
     return m_gameWindowHandle != nullptr && IsGameWindowValid();
 }
 
 HWND WindowManager::GetGameWindowHandle() const {
     return m_gameWindowHandle;
 }
 
 OverlayWindow& WindowManager::GetOverlayWindow() {
     return m_overlayWindow;
 }
 
 void WindowManager::SetInteractionMode(OverlayWindow::WindowMode mode) {
     m_overlayWindow.SetMode(mode);
 }
 
 void WindowManager::ToggleInteractionMode() {
     if (m_overlayWindow.GetMode() == OverlayWindow::WindowMode::Interactive) {
         m_overlayWindow.SetMode(OverlayWindow::WindowMode::ClickThrough);
     } else {
         m_overlayWindow.SetMode(OverlayWindow::WindowMode::Interactive);
     }
 }
 
 void WindowManager::ToggleVisibility() {
     m_overlayWindow.SetVisible(!m_overlayWindow.IsVisible());
 }
 
 void WindowManager::SetupEventCallbacks() {
     // Set event callback for the overlay window
     m_overlayWindow.SetEventCallback(
         [this](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
             return HandleWindowEvent(hwnd, uMsg, wParam, lParam);
         }
     );
 }
 
 bool WindowManager::IsGameWindowValid() const {
     return m_gameWindowHandle && IsWindow(m_gameWindowHandle);
 }
 
 void WindowManager::UpdateOverlayPosition() {
     if (!m_gameWindowHandle || !IsGameWindowValid()) {
         return;
     }
 
     // Get the game window rect
     RECT gameRect;
     if (!GetWindowRect(m_gameWindowHandle, &gameRect)) {
         return;
     }
 
     // Check if the window is minimized
     if (IsIconic(m_gameWindowHandle)) {
         // Don't update position if the game is minimized
         return;
     }
 
     // Update the overlay position to match the game window
     int width = gameRect.right - gameRect.left;
     int height = gameRect.bottom - gameRect.top;
     
     // Only update if the position or size has changed
     RECT overlayRect = m_overlayWindow.GetBounds();
     if (overlayRect.left != gameRect.left || overlayRect.top != gameRect.top ||
         (overlayRect.right - overlayRect.left) != width || 
         (overlayRect.bottom - overlayRect.top) != height) {
         
         m_overlayWindow.SetBounds(gameRect.left, gameRect.top, width, height);
     }
 }
 
 LRESULT WindowManager::HandleWindowEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
     // Handle window messages
     switch (uMsg) {
         case WM_CLOSE:
             // Intercept close message to handle it properly
             m_running = false;
             return 0;
             
         case WM_SIZE:
             // Handle window resize
             return 0;
             
         case WM_SETFOCUS:
             // The overlay window should never take focus
             if (m_gameWindowHandle && IsGameWindowValid()) {
                 SetFocus(m_gameWindowHandle);
                 return 0;
             }
             break;
             
         case WM_ACTIVATE:
             // Prevent overlay from stealing activation from the game
             if (LOWORD(wParam) != WA_INACTIVE && m_gameWindowHandle && IsGameWindowValid()) {
                 SetFocus(m_gameWindowHandle);
                 return 0;
             }
             break;
     }
     
     // Return 0 to indicate we didn't handle the message
     return 0;
 }
 
 } // namespace poe