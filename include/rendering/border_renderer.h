#pragma once

#include <Windows.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <memory>
#include <string>
#include "core/Application.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace poe {

// Forward declarations
class OverlayWindow;

/**
 * @class BorderRenderer
 * @brief Renders customizable borders around the overlay window.
 * 
 * This class provides rendering of smart borders that highlight
 * when the cursor approaches the window edges.
 */
class BorderRenderer {
public:
    /**
     * @struct BorderStyle
     * @brief Style parameters for the border.
     */
    struct BorderStyle {
        D2D1::ColorF color = D2D1::ColorF(0.2f, 0.6f, 1.0f, 0.6f); ///< Border color
        float thickness = 2.0f;                                     ///< Border thickness in pixels
        float cornerRadius = 3.0f;                                  ///< Corner radius in pixels
        bool drawShadow = true;                                     ///< Whether to draw a shadow
        D2D1::ColorF shadowColor = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.3f); ///< Shadow color
        float shadowBlur = 5.0f;                                    ///< Shadow blur radius
        float shadowOffset = 2.0f;                                  ///< Shadow offset
    };

    /**
     * @brief Constructor for the BorderRenderer class.
     * @param app Reference to the main application instance.
     * @param overlayWindow Reference to the overlay window.
     */
    BorderRenderer(Application& app, OverlayWindow& overlayWindow);
    
    /**
     * @brief Destructor for the BorderRenderer class.
     */
    ~BorderRenderer();
    
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
     * @brief Renders the border.
     * @param rt The render target to draw on.
     * @param opacity The opacity of the border (0.0-1.0).
     */
    void Render(ID2D1RenderTarget* rt, float opacity);
    
    /**
     * @brief Resize the border for the new window dimensions.
     * @param width The new width.
     * @param height The new height.
     */
    void Resize(int width, int height);
    
    /**
     * @brief Set the border style.
     * @param style The new border style.
     */
    void SetStyle(const BorderStyle& style);
    
    /**
     * @brief Get the current border style.
     * @return The current border style.
     */
    const BorderStyle& GetStyle() const { return m_style; }

private:
    /**
     * @brief Creates Direct2D resources for rendering.
     * @return True if creation succeeded, false otherwise.
     */
    bool CreateDeviceResources();
    
    /**
     * @brief Releases Direct2D resources.
     */
    void ReleaseDeviceResources();
    
    /**
     * @brief Log a message using the application logger.
     * @tparam Args Variadic template for format arguments.
     * @param level The log level (0=trace, 1=debug, 2=info, 3=warning, 4=error, 5=critical).
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    void Log(int level, const std::string& fmt, const Args&... args);

    Application& m_app;                           ///< Reference to the main application
    OverlayWindow& m_overlayWindow;               ///< Reference to the overlay window
    
    // Border style
    BorderStyle m_style;                          ///< Current border style
    
    // Direct2D resources
    Microsoft::WRL::ComPtr<ID2D1Factory> m_d2dFactory; ///< Direct2D factory
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_borderBrush; ///< Border brush
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_shadowBrush; ///< Shadow brush
    
    // State variables
    bool m_initialized;                           ///< Whether the renderer is initialized
    int m_width;                                  ///< Current width
    int m_height;                                 ///< Current height
};

} // namespace poe