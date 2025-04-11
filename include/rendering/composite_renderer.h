#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d2d1.h>
#include <dcomp.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <functional>
#include "core/Application.h"
#include "rendering/overlay_renderer.h"
#include "rendering/border_renderer.h"

namespace poe {

// Forward declarations
class OverlayWindow;
class AnimationManager;

/**
 * @class CompositeRenderer
 * @brief Manages multiple rendering components for the overlay.
 * 
 * This class coordinates the rendering pipeline, combining DirectComposition,
 * Direct2D, and CEF browser rendering into a cohesive output.
 */
class CompositeRenderer {
public:
    /**
     * @brief Constructor for the CompositeRenderer class.
     * @param app Reference to the main application instance.
     * @param overlayWindow Reference to the overlay window.
     */
    CompositeRenderer(Application& app, OverlayWindow& overlayWindow);
    
    /**
     * @brief Destructor for the CompositeRenderer class.
     */
    ~CompositeRenderer();
    
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
     * @brief Render a frame.
     */
    void Render();
    
    /**
     * @brief Resize the rendering surface.
     * @param width The new width.
     * @param height The new height.
     */
    void Resize(int width, int height);
    
    /**
     * @brief Set the opacity of the overlay.
     * @param opacity Opacity value between 0.0 (transparent) and 1.0 (opaque).
     */
    void SetOpacity(float opacity);
    
    /**
     * @brief Show or hide the border.
     * @param show Whether to show the border.
     */
    void ShowBorder(bool show);
    
    /**
     * @brief Set the window position.
     * @param x X coordinate.
     * @param y Y coordinate.
     */
    void SetPosition(int x, int y);
    
    /**
     * @brief Get the overlay renderer.
     * @return Reference to the overlay renderer.
     */
    OverlayRenderer& GetOverlayRenderer() { return *m_overlayRenderer; }
    
    /**
     * @brief Get the border renderer.
     * @return Reference to the border renderer.
     */
    BorderRenderer& GetBorderRenderer() { return *m_borderRenderer; }
    
    /**
     * @brief Set the animation manager.
     * @param animationManager Pointer to the animation manager.
     */
    void SetAnimationManager(AnimationManager* animationManager) { m_animationManager = animationManager; }

private:
    /**
     * @brief Create the render target for drawing.
     * @return True if creation succeeded, false otherwise.
     */
    bool CreateRenderTarget();
    
    /**
     * @brief Release the render target.
     */
    void ReleaseRenderTarget();
    
    /**
     * @brief Log a message using the application logger.
     * @tparam Args Variadic template for format arguments.
     * @param level The log level (0=trace, 1=debug, 2=info, 3=warning, 4=error, 5=critical).
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    void Log(int level, const std::string& fmt, const Args&... args);

    Application& m_app;                                 ///< Reference to the main application
    OverlayWindow& m_overlayWindow;                     ///< Reference to the overlay window
    
    // Rendering components
    std::unique_ptr<OverlayRenderer> m_overlayRenderer; ///< Overlay renderer
    std::unique_ptr<BorderRenderer> m_borderRenderer;   ///< Border renderer
    AnimationManager* m_animationManager;               ///< Animation manager (not owned)
    
    // Direct2D resources
    Microsoft::WRL::ComPtr<ID2D1Factory> m_d2dFactory;  ///< Direct2D factory
    Microsoft::WRL::ComPtr<ID2D1RenderTarget> m_renderTarget; ///< Direct2D render target
    
    // State variables
    bool m_initialized;                                 ///< Whether the renderer is initialized
    int m_width;                                        ///< Current width
    int m_height;                                       ///< Current height
    float m_opacity;                                    ///< Current opacity
    bool m_showBorder;                                  ///< Whether to show the border
};

} // namespace poe