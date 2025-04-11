#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dcomp.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <functional>
#include "core/Application.h"

namespace poe {

// Forward declarations
class OverlayWindow;

/**
 * @class OverlayRenderer
 * @brief Manages DirectComposition-based rendering for overlay windows.
 * 
 * This class provides advanced rendering capabilities for the overlay window,
 * including hardware-accelerated transparency, smooth animations, and efficient
 * composition with the desktop.
 */
class OverlayRenderer {
public:
    /**
     * @brief Constructor for the OverlayRenderer class.
     * @param app Reference to the main application instance.
     * @param overlayWindow Reference to the overlay window to render.
     */
    OverlayRenderer(Application& app, OverlayWindow& overlayWindow);
    
    /**
     * @brief Destructor for the OverlayRenderer class.
     */
    ~OverlayRenderer();

    /**
     * @brief Initializes the renderer.
     * @return True if initialization succeeded, false otherwise.
     */
    bool Initialize();
    
    /**
     * @brief Shuts down the renderer and releases resources.
     */
    void Shutdown();

    /**
     * @brief Updates the opacity of the overlay.
     * @param opacity New opacity value (0.0-1.0).
     * @param animate Whether to animate the transition.
     */
    void SetOpacity(float opacity, bool animate = false);
    
    /**
     * @brief Renders a frame.
     */
    void Render();
    
    /**
     * @brief Handles window size changes.
     * @param width New width of the window.
     * @param height New height of the window.
     */
    void Resize(int width, int height);
    
    /**
     * @brief Shows/highlights borders when cursor is near edges.
     * @param show Whether to show the borders.
     */
    void ShowBorders(bool show);
    
    /**
     * @brief Sets a callback to be notified when border state changes.
     * @param callback The callback function.
     */
    void SetBorderStateCallback(std::function<void(bool)> callback) { m_borderStateCallback = callback; }
    
    /**
     * @brief Updates the window position in the composition.
     * @param x X coordinate.
     * @param y Y coordinate.
     */
    void UpdatePosition(int x, int y);

private:
    /**
     * @brief Creates Direct3D and DirectComposition resources.
     * @return True if creation succeeded, false otherwise.
     */
    bool CreateDeviceResources();
    
    /**
     * @brief Creates Direct2D resources for rendering.
     * @return True if creation succeeded, false otherwise.
     */
    bool CreateRenderResources();
    
    /**
     * @brief Sets up the composition tree.
     * @return True if setup succeeded, false otherwise.
     */
    bool SetupComposition();
    
    /**
     * @brief Log a message using the application logger.
     * @tparam Args Variadic template for format arguments.
     * @param level The log level (0=trace, 1=debug, 2=info, 3=warning, 4=error, 5=critical).
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    void Log(int level, const std::string& fmt, const Args&... args);

    // Application reference
    Application& m_app;
    
    // Overlay window reference
    OverlayWindow& m_overlayWindow;
    
    // DirectX resources
    Microsoft::WRL::ComPtr<ID3D11Device> m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3dContext;
    Microsoft::WRL::ComPtr<IDXGIDevice> m_dxgiDevice;
    Microsoft::WRL::ComPtr<IDXGIFactory2> m_dxgiFactory;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
    
    // DirectComposition resources
    Microsoft::WRL::ComPtr<IDCompositionDevice> m_dcompDevice;
    Microsoft::WRL::ComPtr<IDCompositionTarget> m_dcompTarget;
    Microsoft::WRL::ComPtr<IDCompositionVisual> m_rootVisual;
    Microsoft::WRL::ComPtr<IDCompositionVisual> m_contentVisual;
    Microsoft::WRL::ComPtr<IDCompositionVisual> m_borderVisual;
    
    // State variables
    bool m_initialized;
    float m_currentOpacity;
    float m_targetOpacity;
    bool m_showBorders;
    int m_width;
    int m_height;
    
    // Border detection
    bool m_mouseNearBorder;
    std::function<void(bool)> m_borderStateCallback;
};

} // namespace poe