/**
 * @file overlay_window.cpp
 * @brief Implementation of the OverlayWindow class
 */

 #include "window/overlay_window.h"

 #include <stdexcept>
 #include <string>
 #include <windowsx.h>
 #include <ShellScalingApi.h>
 #include <dwmapi.h>
 #include "core/Logger.h"
 #include "core/ErrorHandler.h"
 
 #pragma comment(lib, "dwmapi.lib")
 #pragma comment(lib, "Shcore.lib")
 
 namespace poe {
 
 // Window class name
 static constexpr wchar_t WINDOW_CLASS_NAME[] = L"PoEOverlayWindowClass";
 
 // Register the window class for the application
 static bool RegisterWindowClass(HINSTANCE hInstance) {
     WNDCLASSEXW wcex = {};
     wcex.cbSize = sizeof(WNDCLASSEXW);
     wcex.style = CS_HREDRAW | CS_VREDRAW;
     wcex.lpfnWndProc = OverlayWindow::WindowProc;
     wcex.hInstance = hInstance;
     wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
     wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
     wcex.lpszClassName = WINDOW_CLASS_NAME;
 
     return RegisterClassExW(&wcex) != 0;
 }
 
 // OverlayWindow implementation
 OverlayWindow::OverlayWindow(Application& app, const WindowConfig& config) 
     : m_app(app),
       m_config(config), 
       m_mode(config.initialMode),
       m_opacity(config.opacity) {
     
     m_instanceHandle = GetModuleHandleW(nullptr);
     
     // Register window class if not registered yet
     static bool classRegistered = RegisterWindowClass(m_instanceHandle);
     if (!classRegistered) {
         m_app.GetErrorHandler().ReportError(
             ErrorSeverity::Error,
             "Failed to register window class",
             "OverlayWindow"
         );
     }
     
     Log(2, "OverlayWindow created with {}x{} dimensions", config.width, config.height);
 }
 
 OverlayWindow::~OverlayWindow() {
     if (m_windowHandle) {
         DestroyWindow(m_windowHandle);
         m_windowHandle = nullptr;
     }
 }
 
 OverlayWindow::OverlayWindow(OverlayWindow&& other) noexcept
     : m_config(std::move(other.m_config)),
       m_windowHandle(other.m_windowHandle),
       m_instanceHandle(other.m_instanceHandle),
       m_visible(other.m_visible),
       m_mode(other.m_mode),
       m_opacity(other.m_opacity),
       m_bounds(other.m_bounds),
       m_eventCallback(std::move(other.m_eventCallback)),
       m_compositionEnabled(other.m_compositionEnabled) {
     
     other.m_windowHandle = nullptr;
 }
 
 OverlayWindow& OverlayWindow::operator=(OverlayWindow&& other) noexcept {
     if (this != &other) {
         if (m_windowHandle) {
             DestroyWindow(m_windowHandle);
         }
         
         m_config = std::move(other.m_config);
         m_windowHandle = other.m_windowHandle;
         m_instanceHandle = other.m_instanceHandle;
         m_visible = other.m_visible;
         m_mode = other.m_mode;
         m_opacity = other.m_opacity;
         m_bounds = other.m_bounds;
         m_eventCallback = std::move(other.m_eventCallback);
         m_compositionEnabled = other.m_compositionEnabled;
         
         other.m_windowHandle = nullptr;
     }
     return *this;
 }
 
 bool OverlayWindow::Create() {
     if (m_windowHandle) {
         return true;  // Already created
     }
 
     try {
         Log(2, "Creating overlay window with title '{}'", std::string(m_config.title.begin(), m_config.title.end()));
         
         // Create the window
         m_windowHandle = CreateWindowExW(
             WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
             WINDOW_CLASS_NAME,
             m_config.title.c_str(),
             WS_POPUP | WS_VISIBLE,
             CW_USEDEFAULT, CW_USEDEFAULT,
             m_config.width, m_config.height,
             nullptr,                    // No parent window
             nullptr,                    // No menu
             m_instanceHandle,
             this                        // Pass 'this' pointer as user data
         );
 
         if (!m_windowHandle) {
             DWORD error = GetLastError();
             m_app.GetErrorHandler().ReportError(
                 ErrorSeverity::Error,
                 "Failed to create overlay window",
                 "OverlayWindow",
                 "Error code: " + std::to_string(error)
             );
             return false;
         }
     }
     catch (const std::exception& ex) {
         m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "OverlayWindow");
         return false;
     }
 
     // Initialize DirectComposition for better rendering
     if (!InitializeComposition()) {
         // Fallback to basic layered window if composition fails
         SetLayeredWindowAttributes(m_windowHandle, 0, static_cast<BYTE>(m_opacity * 255), LWA_ALPHA);
     }
 
     // Setup initial window state
     ApplyOverlayStyles();
     SetupWindowVisuals();
     UpdateClickThrough();
 
     // Set initial visibility
     SetVisible(m_config.showOnStartup);
 
     return true;
 }
 
 void OverlayWindow::SetVisible(bool visible) {
     if (m_windowHandle && m_visible != visible) {
         ShowWindow(m_windowHandle, visible ? SW_SHOW : SW_HIDE);
         m_visible = visible;
         Log(2, "Overlay visibility set to {}", visible ? "visible" : "hidden");
     }
 }
 
 bool OverlayWindow::IsVisible() const {
     return m_visible;
 }
 
 void OverlayWindow::SetMode(WindowMode mode) {
     if (m_mode != mode) {
         m_mode = mode;
         UpdateClickThrough();
         Log(2, "Overlay mode set to {}", mode == WindowMode::Interactive ? "interactive" : "click-through");
     }
 }
 
 OverlayWindow::WindowMode OverlayWindow::GetMode() const {
     return m_mode;
 }
 
 void OverlayWindow::SetOpacity(float opacity) {
     // Clamp opacity to valid range
     opacity = (opacity < 0.0f) ? 0.0f : (opacity > 1.0f) ? 1.0f : opacity;
     
     if (m_opacity != opacity && m_windowHandle) {
         m_opacity = opacity;
         
         if (m_compositionEnabled) {
             // Update opacity through DWM
             BOOL enabled = TRUE;
             DWM_BLURBEHIND blurBehind = {};
             blurBehind.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
             blurBehind.fEnable = TRUE;
             blurBehind.hRgnBlur = nullptr;
             DwmEnableBlurBehindWindow(m_windowHandle, &blurBehind);
 
             // Use SetLayeredWindowAttributes as fallback opacity control
             SetLayeredWindowAttributes(m_windowHandle, 0, static_cast<BYTE>(m_opacity * 255), LWA_ALPHA);
         } else {
             // Basic layered window opacity
             SetLayeredWindowAttributes(m_windowHandle, 0, static_cast<BYTE>(m_opacity * 255), LWA_ALPHA);
         }
     }
 }
 
 float OverlayWindow::GetOpacity() const {
     return m_opacity;
 }
 
 void OverlayWindow::SetBounds(int x, int y, int width, int height, bool repaint) {
     if (m_windowHandle) {
         MoveWindow(m_windowHandle, x, y, width, height, repaint ? TRUE : FALSE);
         
         // Update internal bounds cache
         GetClientRect(m_windowHandle, &m_bounds);
         ClientToScreen(m_windowHandle, reinterpret_cast<POINT*>(&m_bounds.left));
         ClientToScreen(m_windowHandle, reinterpret_cast<POINT*>(&m_bounds.right));
     }
 }
 
 RECT OverlayWindow::GetBounds() const {
     return m_bounds;
 }
 
 void OverlayWindow::SetEventCallback(WindowEventCallback callback) {
     m_eventCallback = std::move(callback);
 }
 
 HWND OverlayWindow::GetHandle() const {
     return m_windowHandle;
 }
 
 bool OverlayWindow::ProcessMessages() {
     MSG msg = {};
     while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
         if (msg.message == WM_QUIT) {
             return false;
         }
         
         TranslateMessage(&msg);
         DispatchMessage(&msg);
     }
     
     return true;
 }
 
 bool OverlayWindow::IsOverlayingGame(HWND gameWindowHandle) const {
     if (!m_windowHandle || !gameWindowHandle) {
         return false;
     }
     
     RECT gameRect = {};
     GetWindowRect(gameWindowHandle, &gameRect);
     
     RECT overlayRect = {};
     GetWindowRect(m_windowHandle, &overlayRect);
     
     RECT intersection = {};
     return IntersectRect(&intersection, &overlayRect, &gameRect) != FALSE;
 }
 
 bool OverlayWindow::AlignWithGameWindow(HWND gameWindowHandle) {
     if (!m_windowHandle || !gameWindowHandle) {
         return false;
     }
     
     RECT gameRect = {};
     if (!GetWindowRect(gameWindowHandle, &gameRect)) {
         return false;
     }
     
     int width = gameRect.right - gameRect.left;
     int height = gameRect.bottom - gameRect.top;
     
     SetBounds(gameRect.left, gameRect.top, width, height);
     return true;
 }
 
 bool OverlayWindow::InitializeComposition() {
     if (!m_windowHandle) {
         return false;
     }
 
     // Enable DirectComposition
     BOOL enabled = FALSE;
     HRESULT hr = DwmIsCompositionEnabled(&enabled);
     if (FAILED(hr) || !enabled) {
         return false;
     }
 
     // Set window attributes for proper composition
     DWM_BLURBEHIND blurBehind = {};
     blurBehind.dwFlags = DWM_BB_ENABLE;
     blurBehind.fEnable = TRUE;
     blurBehind.hRgnBlur = nullptr;
     
     hr = DwmEnableBlurBehindWindow(m_windowHandle, &blurBehind);
     if (FAILED(hr)) {
         return false;
     }
 
     m_compositionEnabled = true;
     return true;
 }
 
 void OverlayWindow::ApplyOverlayStyles() {
     if (!m_windowHandle) {
         return;
     }
 
     // Apply extended window styles
     LONG exStyle = GetWindowLongW(m_windowHandle, GWL_EXSTYLE);
     exStyle |= WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE;
     SetWindowLongW(m_windowHandle, GWL_EXSTYLE, exStyle);
 
     // Set window as topmost
     SetWindowPos(m_windowHandle, HWND_TOPMOST, 0, 0, 0, 0, 
                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
 }
 
 void OverlayWindow::SetupWindowVisuals() {
     if (!m_windowHandle) {
         return;
     }
 
     // Set window transparency color
     if (!m_compositionEnabled) {
         SetLayeredWindowAttributes(m_windowHandle, 0, static_cast<BYTE>(m_opacity * 255), LWA_ALPHA);
     }
 
     // Set margins to extend frame into client area (required for proper DWM composition)
     MARGINS margins = {-1};  // Extend glass frame to entire window
     DwmExtendFrameIntoClientArea(m_windowHandle, &margins);
 }
 
 void OverlayWindow::UpdateClickThrough() {
     if (!m_windowHandle) {
         return;
     }
 
     LONG exStyle = GetWindowLongW(m_windowHandle, GWL_EXSTYLE);
     
     if (m_mode == WindowMode::ClickThrough) {
         // Enable click-through by adding WS_EX_TRANSPARENT
         exStyle |= WS_EX_TRANSPARENT;
     } else {
         // Disable click-through by removing WS_EX_TRANSPARENT
         exStyle &= ~WS_EX_TRANSPARENT;
     }
     
     SetWindowLongW(m_windowHandle, GWL_EXSTYLE, exStyle);
     
     // Update window to apply the style change
     SetWindowPos(m_windowHandle, nullptr, 0, 0, 0, 0, 
                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
 }
 
 LRESULT CALLBACK OverlayWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
     OverlayWindow* window = nullptr;
     
     if (uMsg == WM_NCCREATE) {
         // Store the OverlayWindow instance in the window user data
         CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
         window = static_cast<OverlayWindow*>(createStruct->lpCreateParams);
         
         if (window) {
             SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
         }
     } else {
         // Retrieve the OverlayWindow instance from window user data
         window = reinterpret_cast<OverlayWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
     }
     
     // Forward the message to the instance method if available
     if (window && window->m_eventCallback) {
         LRESULT result = window->m_eventCallback(hwnd, uMsg, wParam, lParam);
         if (result != 0) {
             return result;
         }
     }
     
     // Handle specific messages here
     switch (uMsg) {
         case WM_DESTROY:
             SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
             PostQuitMessage(0);
             return 0;
             
         case WM_NCHITTEST:
             // Make the entire window draggable
             return HTCAPTION;
     }
     
     return DefWindowProcW(hwnd, uMsg, wParam, lParam);
 }
 
 } // namespace poe