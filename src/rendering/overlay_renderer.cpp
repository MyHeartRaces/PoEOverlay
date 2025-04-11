#include "rendering/overlay_renderer.h"
#include "window/overlay_window.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"

#include <stdexcept>
#include <chrono>

// DirectX libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "dxgi.lib")

namespace poe {

OverlayRenderer::OverlayRenderer(Application& app, OverlayWindow& overlayWindow)
    : m_app(app)
    , m_overlayWindow(overlayWindow)
    , m_initialized(false)
    , m_currentOpacity(1.0f)
    , m_targetOpacity(1.0f)
    , m_showBorders(false)
    , m_width(0)
    , m_height(0)
    , m_mouseNearBorder(false)
{
    Log(2, "OverlayRenderer created");
}

OverlayRenderer::~OverlayRenderer()
{
    Shutdown();
}

bool OverlayRenderer::Initialize()
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

        // Create DirectX resources
        if (!CreateDeviceResources()) {
            Log(4, "Failed to create device resources");
            return false;
        }

        // Create rendering resources
        if (!CreateRenderResources()) {
            Log(4, "Failed to create render resources");
            return false;
        }

        // Setup composition
        if (!SetupComposition()) {
            Log(4, "Failed to setup composition");
            return false;
        }

        m_initialized = true;
        Log(2, "OverlayRenderer initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        m_app.GetErrorHandler().ReportException(e, ErrorSeverity::Error, "OverlayRenderer");
        return false;
    }
}

void OverlayRenderer::Shutdown()
{
    if (!m_initialized) {
        return;
    }

    // Release resources in reverse order of creation
    m_borderVisual.Reset();
    m_contentVisual.Reset();
    m_rootVisual.Reset();
    m_dcompTarget.Reset();
    m_dcompDevice.Reset();
    m_swapChain.Reset();
    m_dxgiFactory.Reset();
    m_dxgiDevice.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();

    m_initialized = false;
    Log(2, "OverlayRenderer shutdown");
}

bool OverlayRenderer::CreateDeviceResources()
{
    HRESULT hr;

    // Create D3D11 device
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    #ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

    hr = D3D11CreateDevice(
        nullptr,                      // Default adapter
        D3D_DRIVER_TYPE_HARDWARE,     // Hardware driver
        nullptr,                      // No software driver
        creationFlags,                // Flags
        featureLevels,                // Feature levels
        ARRAYSIZE(featureLevels),     // Number of feature levels
        D3D11_SDK_VERSION,            // SDK version
        &m_d3dDevice,                 // Output device
        nullptr,                      // Output feature level
        &m_d3dContext                 // Output context
    );

    if (FAILED(hr)) {
        Log(4, "Failed to create D3D11 device: 0x{:X}", hr);
        return false;
    }

    // Get DXGI device
    hr = m_d3dDevice.As(&m_dxgiDevice);
    if (FAILED(hr)) {
        Log(4, "Failed to get DXGI device: 0x{:X}", hr);
        return false;
    }

    // Create DXGI factory
    hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_dxgiFactory));
    if (FAILED(hr)) {
        Log(4, "Failed to create DXGI factory: 0x{:X}", hr);
        return false;
    }

    // Create DirectComposition device
    hr = DCompositionCreateDevice(m_dxgiDevice.Get(), IID_PPV_ARGS(&m_dcompDevice));
    if (FAILED(hr)) {
        Log(4, "Failed to create DirectComposition device: 0x{:X}", hr);
        return false;
    }

    Log(1, "Device resources created successfully");
    return true;
}

bool OverlayRenderer::CreateRenderResources()
{
    if (m_width <= 0 || m_height <= 0) {
        Log(4, "Invalid dimensions for swap chain: {}x{}", m_width, m_height);
        return false;
    }

    HRESULT hr;

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    hr = m_dxgiFactory->CreateSwapChainForComposition(
        m_d3dDevice.Get(),
        &swapChainDesc,
        nullptr,
        &m_swapChain
    );

    if (FAILED(hr)) {
        Log(4, "Failed to create swap chain: 0x{:X}", hr);
        return false;
    }

    Log(1, "Render resources created successfully");
    return true;
}

bool OverlayRenderer::SetupComposition()
{
    HRESULT hr;

    // Create DirectComposition target for the window
    hr = m_dcompDevice->CreateTargetForHwnd(
        m_overlayWindow.GetHandle(),
        TRUE,
        &m_dcompTarget
    );

    if (FAILED(hr)) {
        Log(4, "Failed to create composition target: 0x{:X}", hr);
        return false;
    }

    // Create root visual
    hr = m_dcompDevice->CreateVisual(&m_rootVisual);
    if (FAILED(hr)) {
        Log(4, "Failed to create root visual: 0x{:X}", hr);
        return false;
    }

    // Create content visual
    hr = m_dcompDevice->CreateVisual(&m_contentVisual);
    if (FAILED(hr)) {
        Log(4, "Failed to create content visual: 0x{:X}", hr);
        return false;
    }

    // Create border visual
    hr = m_dcompDevice->CreateVisual(&m_borderVisual);
    if (FAILED(hr)) {
        Log(4, "Failed to create border visual: 0x{:X}", hr);
        return false;
    }

    // Set swap chain on content visual
    hr = m_contentVisual->SetContent(m_swapChain.Get());
    if (FAILED(hr)) {
        Log(4, "Failed to set content on visual: 0x{:X}", hr);
        return false;
    }

    // Set opacity
    hr = m_contentVisual->SetOpacity(m_currentOpacity);
    if (FAILED(hr)) {
        Log(4, "Failed to set opacity: 0x{:X}", hr);
        return false;
    }

    // Add content visual to root
    hr = m_rootVisual->AddVisual(m_contentVisual.Get(), TRUE, nullptr);
    if (FAILED(hr)) {
        Log(4, "Failed to add content visual: 0x{:X}", hr);
        return false;
    }

    // Add border visual to root (initially hidden)
    hr = m_rootVisual->AddVisual(m_borderVisual.Get(), FALSE, m_contentVisual.Get());
    if (FAILED(hr)) {
        Log(4, "Failed to add border visual: 0x{:X}", hr);
        return false;
    }
    
    // Set border opacity to 0 (hidden by default)
    hr = m_borderVisual->SetOpacity(0.0f);
    if (FAILED(hr)) {
        Log(4, "Failed to set border opacity: 0x{:X}", hr);
        return false;
    }

    // Set root visual on target
    hr = m_dcompTarget->SetRoot(m_rootVisual.Get());
    if (FAILED(hr)) {
        Log(4, "Failed to set root visual: 0x{:X}", hr);
        return false;
    }

    // Commit the composition
    hr = m_dcompDevice->Commit();
    if (FAILED(hr)) {
        Log(4, "Failed to commit composition: 0x{:X}", hr);
        return false;
    }

    Log(1, "Composition setup successfully");
    return true;
}

void OverlayRenderer::SetOpacity(float opacity, bool animate)
{
    if (!m_initialized) {
        return;
    }

    // Clamp to valid range
    opacity = (opacity < 0.0f) ? 0.0f : (opacity > 1.0f) ? 1.0f : opacity;

    if (m_targetOpacity == opacity) {
        return; // No change needed
    }

    m_targetOpacity = opacity;

    if (!animate) {
        // Apply immediately
        m_currentOpacity = opacity;
        m_contentVisual->SetOpacity(opacity);
        m_dcompDevice->Commit();
        return;
    }

    // Animation handled in Render() method
    // The opacity will be gradually changed in the render loop
}

void OverlayRenderer::Render()
{
    if (!m_initialized) {
        return;
    }

    // Update opacity animation if needed
    if (m_currentOpacity != m_targetOpacity) {
        const float opacityStep = 0.05f;
        
        if (m_currentOpacity < m_targetOpacity) {
            m_currentOpacity = std::min(m_currentOpacity + opacityStep, m_targetOpacity);
        } else {
            m_currentOpacity = std::max(m_currentOpacity - opacityStep, m_targetOpacity);
        }
        
        m_contentVisual->SetOpacity(m_currentOpacity);
        m_dcompDevice->Commit();
    }

    // Render logic goes here
    // For now we just rely on DirectComposition to present the window
}

void OverlayRenderer::Resize(int width, int height)
{
    if (!m_initialized || width <= 0 || height <= 0 || (width == m_width && height == m_height)) {
        return;
    }

    m_width = width;
    m_height = height;

    // Release swap chain
    m_contentVisual->SetContent(nullptr);
    m_swapChain.Reset();

    // Recreate swap chain with new size
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    HRESULT hr = m_dxgiFactory->CreateSwapChainForComposition(
        m_d3dDevice.Get(),
        &swapChainDesc,
        nullptr,
        &m_swapChain
    );

    if (FAILED(hr)) {
        Log(4, "Failed to resize swap chain: 0x{:X}", hr);
        return;
    }

    // Set swap chain on content visual
    hr = m_contentVisual->SetContent(m_swapChain.Get());
    if (FAILED(hr)) {
        Log(4, "Failed to set content on visual after resize: 0x{:X}", hr);
        return;
    }

    // Commit changes
    m_dcompDevice->Commit();
    Log(1, "Resized renderer to {}x{}", width, height);
}

void OverlayRenderer::ShowBorders(bool show)
{
    if (!m_initialized || m_showBorders == show) {
        return;
    }

    m_showBorders = show;
    
    // Set border opacity based on visibility
    float borderOpacity = show ? 0.7f : 0.0f;
    m_borderVisual->SetOpacity(borderOpacity);
    
    // Commit changes
    m_dcompDevice->Commit();
    
    // Notify via callback if registered
    if (m_borderStateCallback) {
        m_borderStateCallback(show);
    }
}

void OverlayRenderer::UpdatePosition(int x, int y)
{
    if (!m_initialized) {
        return;
    }

    // Update visual position
    m_rootVisual->SetOffsetX(static_cast<float>(x));
    m_rootVisual->SetOffsetY(static_cast<float>(y));
    
    // Commit changes
    m_dcompDevice->Commit();
}

template<typename... Args>
void OverlayRenderer::Log(int level, const std::string& fmt, const Args&... args)
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
        std::cerr << "OverlayRenderer log error: " << e.what() << std::endl;
    }
}

} // namespace poe