#include "rendering/composite_renderer.h"
#include "window/overlay_window.h"
#include "rendering/animation_manager.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"

namespace poe {

CompositeRenderer::CompositeRenderer(Application& app, OverlayWindow& overlayWindow)
    : m_app(app)
    , m_overlayWindow(overlayWindow)
    , m_animationManager(nullptr)
    , m_initialized(false)
    , m_width(0)
    , m_height(0)
    , m_opacity(1.0f)
    , m_showBorder(false)
{
    Log(2, "CompositeRenderer created");
}

CompositeRenderer::~CompositeRenderer()
{
    Shutdown();
}

bool CompositeRenderer::Initialize()
{
    if (m_initialized) {
        return true;
    }

    try {
        // Get window dimensions
        RECT clientRect;
        GetClientRect(m_overlayWindow.GetHandle(), &clientRect);
        m_width = clientRect.right - clientRect.left;
        m_height = clientRect.bottom - clientRect.top;

        // Create overlay renderer
        m_overlayRenderer = std::make_unique<OverlayRenderer>(m_app, m_overlayWindow);
        if (!m_overlayRenderer->Initialize()) {
            Log(4, "Failed to initialize overlay renderer");
            return false;
        }

        // Create border renderer
        m_borderRenderer = std::make_unique<BorderRenderer>(m_app, m_overlayWindow);
        if (!m_borderRenderer->Initialize()) {
            Log(4, "Failed to initialize border renderer");
            return false;
        }

        // Create Direct2D factory
        HRESULT hr = D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            m_d2dFactory.GetAddressOf()
        );

        if (FAILED(hr)) {
            Log(4, "Failed to create Direct2D factory: 0x{:X}", hr);
            return false;
        }

        // Create render target
        if (!CreateRenderTarget()) {
            return false;
        }

        m_initialized = true;
        Log(2, "CompositeRenderer initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        m_app.GetErrorHandler().ReportException(e, ErrorSeverity::Error, "CompositeRenderer");
        return false;
    }
}

void CompositeRenderer::Shutdown()
{
    if (!m_initialized) {
        return;
    }

    ReleaseRenderTarget();
    
    if (m_borderRenderer) {
        m_borderRenderer->Shutdown();
        m_borderRenderer.reset();
    }
    
    if (m_overlayRenderer) {
        m_overlayRenderer->Shutdown();
        m_overlayRenderer.reset();
    }
    
    m_d2dFactory.Reset();

    m_initialized = false;
    Log(2, "CompositeRenderer shutdown");
}

bool CompositeRenderer::CreateRenderTarget()
{
    if (!m_initialized || !m_d2dFactory || m_width <= 0 || m_height <= 0) {
        return false;
    }

    try {
        // Release existing render target if any
        ReleaseRenderTarget();

        // Create HWND render target properties
        D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(
            m_overlayWindow.GetHandle(),
            D2D1::SizeU(m_width, m_height),
            D2D1_PRESENT_OPTIONS_NONE
        );

        // Create render target properties
        D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.0f, 96.0f,
            D2D1_RENDER_TARGET_USAGE_NONE,
            D2D1_FEATURE_LEVEL_DEFAULT
        );

        // Create render target
        Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> hwndRT;
        HRESULT hr = m_d2dFactory->CreateHwndRenderTarget(
            rtProps,
            hwndProps,
            &hwndRT
        );

        if (FAILED(hr)) {
            Log(4, "Failed to create HWND render target: 0x{:X}", hr);
            return false;
        }

        // Store as ID2D1RenderTarget
        m_renderTarget = hwndRT;

        Log(1, "Render target created successfully");
        return true;
    }
    catch (const std::exception& e) {
        m_app.GetErrorHandler().ReportException(e, ErrorSeverity::Error, "CompositeRenderer");
        return false;
    }
}

void CompositeRenderer::ReleaseRenderTarget()
{
    m_renderTarget.Reset();
}

void CompositeRenderer::Render()
{
    if (!m_initialized) {
        return;
    }

    // Update overlay renderer
    if (m_overlayRenderer) {
        m_overlayRenderer->Render();
    }

    // Draw border if needed using Direct2D
    if (m_showBorder && m_borderRenderer && m_renderTarget) {
        m_renderTarget->BeginDraw();
        
        // Clear the background (transparent)
        m_renderTarget->Clear(D2D1::ColorF(0, 0, 0, 0));
        
        // Render border
        m_borderRenderer->Render(m_renderTarget.Get(), m_opacity);
        
        HRESULT hr = m_renderTarget->EndDraw();
        if (FAILED(hr)) {
            Log(4, "Failed to end drawing: 0x{:X}", hr);
            ReleaseRenderTarget();
            CreateRenderTarget(); // Try to recover
        }
    }
}

void CompositeRenderer::Resize(int width, int height)
{
    if (!m_initialized || width <= 0 || height <= 0 || (width == m_width && height == m_height)) {
        return;
    }

    m_width = width;
    m_height = height;

    // Resize overlay renderer
    if (m_overlayRenderer) {
        m_overlayRenderer->Resize(width, height);
    }

    // Resize border renderer
    if (m_borderRenderer) {
        m_borderRenderer->Resize(width, height);
    }

    // Recreate render target
    CreateRenderTarget();
}

void CompositeRenderer::SetOpacity(float opacity)
{
    // Clamp opacity to valid range
    opacity = (opacity < 0.0f) ? 0.0f : (opacity > 1.0f) ? 1.0f : opacity;
    
    if (m_opacity != opacity) {
        m_opacity = opacity;
        
        // Update overlay renderer opacity
        if (m_overlayRenderer) {
            m_overlayRenderer->SetOpacity(opacity, false);
        }
    }
}

void CompositeRenderer::ShowBorder(bool show)
{
    if (m_showBorder != show) {
        m_showBorder = show;
        
        // Update overlay renderer
        if (m_overlayRenderer) {
            m_overlayRenderer->ShowBorders(show);
        }
    }
}

void CompositeRenderer::SetPosition(int x, int y)
{
    // Update overlay renderer position
    if (m_overlayRenderer) {
        m_overlayRenderer->UpdatePosition(x, y);
    }
}

template<typename... Args>
void CompositeRenderer::Log(int level, const std::string& fmt, const Args&... args)
{
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
        std::cerr << "CompositeRenderer log error: " << e.what() << std::endl;
    }
}

} // namespace poe