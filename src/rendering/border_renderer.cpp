#include "rendering/border_renderer.h"
#include "window/overlay_window.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"

namespace poe {

BorderRenderer::BorderRenderer(Application& app, OverlayWindow& overlayWindow)
    : m_app(app)
    , m_overlayWindow(overlayWindow)
    , m_initialized(false)
    , m_width(0)
    , m_height(0)
{
    Log(2, "BorderRenderer created");
}

BorderRenderer::~BorderRenderer()
{
    Shutdown();
}

bool BorderRenderer::Initialize()
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

        // Create Direct2D factory
        HRESULT hr = D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            m_d2dFactory.GetAddressOf()
        );

        if (FAILED(hr)) {
            Log(4, "Failed to create Direct2D factory: 0x{:X}", hr);
            return false;
        }

        m_initialized = true;
        Log(2, "BorderRenderer initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        m_app.GetErrorHandler().ReportException(e, ErrorSeverity::Error, "BorderRenderer");
        return false;
    }
}

void BorderRenderer::Shutdown()
{
    if (!m_initialized) {
        return;
    }

    ReleaseDeviceResources();
    m_d2dFactory.Reset();

    m_initialized = false;
    Log(2, "BorderRenderer shutdown");
}

void BorderRenderer::Render(ID2D1RenderTarget* rt, float opacity)
{
    if (!m_initialized || !rt) {
        return;
    }

    // Create resources if needed
    if (!m_borderBrush || !m_shadowBrush) {
        HRESULT hr;

        // Create border brush
        D2D1::ColorF borderColor = m_style.color;
        borderColor.a *= opacity; // Apply opacity
        hr = rt->CreateSolidColorBrush(
            borderColor,
            m_borderBrush.GetAddressOf()
        );

        if (FAILED(hr)) {
            Log(4, "Failed to create border brush: 0x{:X}", hr);
            return;
        }

        // Create shadow brush
        D2D1::ColorF shadowColor = m_style.shadowColor;
        shadowColor.a *= opacity; // Apply opacity
        hr = rt->CreateSolidColorBrush(
            shadowColor,
            m_shadowBrush.GetAddressOf()
        );

        if (FAILED(hr)) {
            Log(4, "Failed to create shadow brush: 0x{:X}", hr);
            return;
        }
    }

    // Begin drawing
    rt->BeginDraw();

    // Set transform to identity
    rt->SetTransform(D2D1::Matrix3x2F::Identity());

    // Create border rectangle with rounded corners
    D2D1_ROUNDED_RECT borderRect = {
        D2D1::RectF(
            m_style.thickness / 2,
            m_style.thickness / 2,
            m_width - m_style.thickness / 2,
            m_height - m_style.thickness / 2
        ),
        m_style.cornerRadius,
        m_style.cornerRadius
    };

    // Draw shadow first if enabled
    if (m_style.drawShadow) {
        // Create shadow rectangle (slightly larger and offset)
        D2D1_ROUNDED_RECT shadowRect = {
            D2D1::RectF(
                m_style.thickness / 2 + m_style.shadowOffset,
                m_style.thickness / 2 + m_style.shadowOffset,
                m_width - m_style.thickness / 2 + m_style.shadowOffset,
                m_height - m_style.thickness / 2 + m_style.shadowOffset
            ),
            m_style.cornerRadius,
            m_style.cornerRadius
        };

        // Draw shadow
        rt->DrawRoundedRectangle(shadowRect, m_shadowBrush.Get(), m_style.thickness);
    }

    // Draw border
    rt->DrawRoundedRectangle(borderRect, m_borderBrush.Get(), m_style.thickness);

    // End drawing
    HRESULT hr = rt->EndDraw();
    if (FAILED(hr)) {
        Log(4, "Failed to end drawing: 0x{:X}", hr);
        ReleaseDeviceResources();
    }
}

void BorderRenderer::Resize(int width, int height)
{
    if (!m_initialized || (width == m_width && height == m_height)) {
        return;
    }

    m_width = width;
    m_height = height;

    // Release device resources to be recreated with new size
    ReleaseDeviceResources();
}

void BorderRenderer::SetStyle(const BorderStyle& style)
{
    if (!m_initialized) {
        return;
    }

    m_style = style;

    // Release device resources to be recreated with new style
    ReleaseDeviceResources();
}

bool BorderRenderer::CreateDeviceResources()
{
    if (!m_initialized) {
        return false;
    }

    // Resources will be created on-demand in Render
    return true;
}

void BorderRenderer::ReleaseDeviceResources()
{
    m_borderBrush.Reset();
    m_shadowBrush.Reset();
}

template<typename... Args>
void BorderRenderer::Log(int level, const std::string& fmt, const Args&... args)
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
        std::cerr << "BorderRenderer log error: " << e.what() << std::endl;
    }
}

} // namespace poe