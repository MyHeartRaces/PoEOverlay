#include "window/overlay_window.h"

#include <stdexcept>
#include <string>
#include <windowsx.h>
#include <ShellScalingApi.h>
#include <dwmapi.h>
#include "core/Logger.h"
#include "core/ErrorHandler.h"
#include "rendering/overlay_renderer.h"
#include "rendering/animation_manager.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Shcore.lib")

namespace poe {

// Window class name
static constexpr wchar_t WINDOW_CLASS_NAME[] = L"PoEOverlayWindowClass";

// Custom window messages
enum {
    WM_UPDATE_BORDER = WM_USER + 100,
    WM_CHECK_MOUSE_POSITION
};

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
    // Clean up renderer and animation manager first
    m_renderer.reset();
    m_animationManager.reset();
    
    if (m_windowHandle) {
        DestroyWindow(m_windowHandle);
        m_windowHandle = nullptr;
    }
}

OverlayWindow::OverlayWindow(OverlayWindow&& other) noexcept
    : m_app(other.m_app),
      m_config(std::move(other.m_config)),
      m_windowHandle(other.m_windowHandle),
      m_instanceHandle(other.m_instanceHandle),
      m_visible(other.m_visible),
      m_mode(other.m_mode),
      m_opacity(other.m_opacity),
      m_bounds(other.m_bounds),
      m_eventCallback(std::move(other.m_eventCallback)),
      m_mouseTracking(other.m_mouseTracking),
      m_mouseNearEdge(other.m_mouseNearEdge),
      m_lastMousePos(other.m_lastMousePos),
      m_renderer(std::move(other.m_renderer)),
      m_animationManager(std::move(other.m_animationManager)),
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
        m_mouseTracking = other.m_mouseTracking;
        m_mouseNearEdge = other.m_mouseNearEdge;
        m_lastMousePos = other.m_lastMousePos;
        m_renderer = std::move(other.m_renderer);
        m_animationManager = std::move(other.m_animationManager);
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

        // Create renderer
        m_renderer = std::make_unique<OverlayRenderer>(m_app, *this);
        if (!m_renderer->Initialize()) {
            m_app.GetErrorHandler().ReportError(
                ErrorSeverity::Error,
                "Failed to initialize overlay renderer",
                "OverlayWindow"
            );
            return false;
        }

        // Create animation manager
        m_animationManager = std::make_unique<AnimationManager>(m_app);
        if (!m_animationManager->Initialize()) {
            m_app.GetErrorHandler().ReportError(
                ErrorSeverity::Error,
                "Failed to initialize animation manager",
                "OverlayWindow"
            );
            return false;
        }

        // Setup animations
        SetupAnimations();

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

        // Setup mouse tracking
        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE | TME_HOVER;
        tme.hwndTrack = m_windowHandle;
        tme.dwHoverTime = HOVER_DEFAULT;
        m_mouseTracking = TrackMouseEvent(&tme) != FALSE;

        // Set a timer to periodically check mouse position for border highlighting
        SetTimer(m_windowHandle, 1, 100, nullptr);

        return true;
    }
    catch (const std::exception& ex) {
        m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "OverlayWindow");
        return false;
    }
}

void OverlayWindow::SetupAnimations() {
    if (!m_animationManager) {
        return;
    }

    // Create opacity animation
    m_animationManager->CreateFloatAnimation(
        "opacity",
        300, // 300ms duration
        m_opacity,
        m_opacity,
        [this](float value) {
            m_opacity = value;
            if (m_renderer) {
                m_renderer->SetOpacity(value, false);
            } else {
                // Fallback to basic layered window opacity
                SetLayeredWindowAttributes(m_windowHandle, 0, static_cast<BYTE>(value * 255), LWA_ALPHA);
            }
        }
    );

    // Create border animation
    m_animationManager->CreateFloatAnimation(
        "border",
        200, // 200ms duration
        0.0f,
        1.0f,
        [this](float value) {
            if (m_renderer) {
                m_renderer->ShowBorders(value > 0.01f);
            }
        }
    );
}

void OverlayWindow::SetVisible(bool visible, bool animate) {
    if (m_windowHandle && m_visible != visible) {
        if (animate && m_animationManager) {
            // Create or update the opacity animation
            auto opacityAnim = m_animationManager->CreateFloatAnimation(
                "opacity",
                300, // 300ms duration
                m_opacity,
                visible ? m_config.opacity : 0.0f,
                [this](float value) {
                    m_opacity = value;
                    if (m_renderer) {
                        m_renderer->SetOpacity(value, false);
                    } else {
                        // Fallback to basic layered window opacity
                        SetLayeredWindowAttributes(m_windowHandle, 0, static_cast<BYTE>(value * 255), LWA_ALPHA);
                    }
                    
                    // Hide window completely when opacity reaches 0
                    if (value < 0.01f && m_visible) {
                        ShowWindow(m_windowHandle, SW_HIDE);
                        m_visible = false;
                    }
                }
            );
            
            // Start the animation
            m_animationManager->StartAnimation("opacity");
            
            // If showing, make the window visible immediately
            if (visible && !m_visible) {
                ShowWindow(m_windowHandle, SW_SHOW);
                m_visible = true;
            }
        } else {
            // No animation, just show/hide the window
            ShowWindow(m_windowHandle, visible ? SW_SHOW : SW_HIDE);
            m_visible = visible;
            
            // Set opacity directly
            m_opacity = visible ? m_config.opacity : 0.0f;
            if (m_renderer) {
                m_renderer->SetOpacity(m_opacity, false);
            } else {
                SetLayeredWindowAttributes(m_windowHandle, 0, static_cast<BYTE>(m_opacity * 255), LWA_ALPHA);
            }
        }
        
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

void OverlayWindow::SetOpacity(float opacity, bool animate) {
    // Clamp opacity to valid range
    opacity = (opacity < 0.0f) ? 0.0f : (opacity > 1.0f) ? 1.0f : opacity;
    
    if (m_opacity != opacity && m_windowHandle) {
        if (animate && m_animationManager) {
            // Create or update the opacity animation
            auto opacityAnim = m_animationManager->CreateFloatAnimation(
                "opacity",
                300, // 300ms duration
                m_opacity,
                opacity,
                [this](float value) {
                    m_opacity = value;
                    if (m_renderer) {
                        m_renderer->SetOpacity(value, false);
                    } else {
                        // Fallback to basic layered window opacity
                        SetLayeredWindowAttributes(m_windowHandle, 0, static_cast<BYTE>(value * 255), LWA_ALPHA);
                    }
                }
            );
            
            // Start the animation
            m_animationManager->StartAnimation("opacity");
        } else {
            m_opacity = opacity;
            
            if (m_renderer) {
                m_renderer->SetOpacity(opacity, false);
            } else {
                // Fallback to basic layered window opacity
                SetLayeredWindowAttributes(m_windowHandle, 0, static_cast<BYTE>(opacity * 255), LWA_ALPHA);
            }
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
        
        // Update renderer if available
        if (m_renderer) {
            m_renderer->Resize(width, height);
            m_renderer->UpdatePosition(x, y);
        }
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

void OverlayWindow::Update() {
    // Update animations
    if (m_animationManager) {
        m_animationManager->Update();
    }
    
    // Update border highlight based on mouse position
    UpdateBorderHighlight();
    
    // Render the overlay
    if (m_renderer) {
        m_renderer->Render();
    }
}

bool OverlayWindow::IsMouseNearEdge(int x, int y, int threshold) const {
    if (!m_windowHandle) {
        return false;
    }
    
    RECT rect;
    GetClientRect(m_windowHandle, &rect);
    
    return x <= threshold || y <= threshold || 
           x >= rect.right - threshold || y >= rect.bottom - threshold;
}

bool OverlayWindow::InitializeComposition() {
    if (!m_windowHandle) {
        return false;
    }

    // Use the renderer for composition if available
    if (m_renderer) {
        m_compositionEnabled = true;
        return true;
    }

    // Otherwise, basic DWM composition
    try {
        // Check if DWM composition is enabled
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
        
        // Set margins to extend frame into client area
        MARGINS margins = {-1};  // Extend frame to entire window
        hr = DwmExtendFrameIntoClientArea(m_windowHandle, &margins);
        if (FAILED(hr)) {
            return false;
        }
        
        m_compositionEnabled = true;
        return true;
    }
    catch (const std::exception& e) {
        m_app.GetErrorHandler().ReportException(e, ErrorSeverity::Error, "OverlayWindow");
        return false;
    }
}

template<typename... Args>
void OverlayWindow::Log(int level, const std::string& fmt, const Args&... args) {
    try {
        auto& logger = m_app.GetLogger();
        
        switch (level) {
            case 0: logger.Trace(fmt, args...); break;
            case 1: logger.Debug(fmt, args...); break;
            case 2: logger.Info(fmt, args...); break;
            case 3: logger.Warning(fmt, args...); break;
            case 4: logger.Error(fmt, args...); break;
            case 5: logger.Critical(fmt, args...); break;
            default: logger.Info(fmt, args...); break;
        }
    }
    catch (const std::exception& e) {
        // Fallback to stderr if logger is unavailable
        std::cerr << "OverlayWindow log error: " << e.what() << std::endl;
    }
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

    // Set window transparency
    if (!m_renderer) {
        SetLayeredWindowAttributes(m_windowHandle, 0, static_cast<BYTE>(m_opacity * 255), LWA_ALPHA);
    }
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

void OverlayWindow::UpdateBorderHighlight() {
    if (!m_windowHandle || !m_renderer) {
        return;
    }
    
    // Get current mouse position
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(m_windowHandle, &pt);
    
    // Check if mouse is inside window client area
    RECT rect;
    GetClientRect(m_windowHandle, &rect);
    bool mouseInWindow = PtInRect(&rect, pt);
    
    if (mouseInWindow) {
        // Check if mouse is near the edge
        bool nearEdge = IsMouseNearEdge(pt.x, pt.y);
        
        if (nearEdge != m_mouseNearEdge) {
            m_mouseNearEdge = nearEdge;
            
            // Show/hide border based on mouse position
            if (m_animationManager) {
                auto borderAnim = m_animationManager->CreateFloatAnimation(
                    "border",
                    200, // 200ms duration
                    nearEdge ? 0.0f : 1.0f, // Start value
                    nearEdge ? 1.0f : 0.0f, // End value
                    [this](float value) {
                        if (m_renderer) {
                            m_renderer->ShowBorders(value > 0.01f);
                        }
                    }
                );
                
                m_animationManager->StartAnimation("border");
            } else {
                m_renderer->ShowBorders(nearEdge);
            }
        }
    } else if (m_mouseNearEdge) {
        // Mouse left window, hide borders
        m_mouseNearEdge = false;
        
        if (m_animationManager) {
            auto borderAnim = m_animationManager->CreateFloatAnimation(
                "border",
                200, // 200ms duration
                1.0f, // Start value
                0.0f, // End value
                [this](float value) {
                    if (m_renderer) {
                        m_renderer->ShowBorders(value > 0.01f);
                    }
                }
            );
            
            m_animationManager->StartAnimation("border");
        } else {
            m_renderer->ShowBorders(false);
        }
    }
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
            
        case WM_MOUSEMOVE:
            if (window) {
                // Track mouse position for border highlighting
                window->m_lastMousePos.x = GET_X_LPARAM(lParam);
                window->m_lastMousePos.y = GET_Y_LPARAM(lParam);
                
                // Trigger border update
                if (!window->m_mouseTracking) {
                    TRACKMOUSEEVENT tme = {};
                    tme.cbSize = sizeof(TRACKMOUSEEVENT);
                    tme.dwFlags = TME_LEAVE | TME_HOVER;
                    tme.hwndTrack = hwnd;
                    tme.dwHoverTime = HOVER_DEFAULT;
                    window->m_mouseTracking = TrackMouseEvent(&tme) != FALSE;
                }
            }
            break;
            
        case WM_MOUSELEAVE:
            if (window) {
                window->m_mouseTracking = false;
                
                // Mouse left window, hide borders
                if (window->m_mouseNearEdge && window->m_renderer) {
                    window->m_mouseNearEdge = false;
                    
                    if (window->m_animationManager) {
                        auto borderAnim = window->m_animationManager->CreateFloatAnimation(
                            "border",
                            200, // 200ms duration
                            1.0f, // Start value
                            0.0f, // End value
                            [window](float value) {
                                if (window->m_renderer) {
                                    window->m_renderer->ShowBorders(value > 0.01f);
                                }
                            }
                        );
                        
                        window->m_animationManager->StartAnimation("border");
                    } else {
                        window->m_renderer->ShowBorders(false);
                    }
                }
            }
            break;
            
        case WM_TIMER:
            if (window && wParam == 1) {
                // Timer for checking mouse position
                window->UpdateBorderHighlight();
            }
            break;
            
        case WM_SIZE:
            if (window && window->m_renderer) {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                window->m_renderer->Resize(width, height);
            }
            break;
    }
    
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

} // namespace poe