#pragma once

#include <Windows.h>
#include <dcomp.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include "core/Application.h"

namespace poe {

/**
 * @class ZOrderManager
 * @brief Manages the Z-order of visual elements in the overlay.
 * 
 * This class provides management of composition visual elements,
 * including their relative ordering, visibility, and updates.
 */
class ZOrderManager {
public:
    /**
     * @enum LayerType
     * @brief Predefined layer types for common visual elements.
     */
    enum class LayerType {
        Background,     ///< Background layer (bottom)
        Content,        ///< Main content layer
        UI,             ///< UI elements layer
        Popup,          ///< Popup elements layer
        Border,         ///< Border layer
        Foreground,     ///< Foreground layer (top)
        Custom          ///< Custom layer (Z-order specified explicitly)
    };

    /**
     * @brief Constructor for the ZOrderManager class.
     * @param app Reference to the main application instance.
     * @param dcompDevice DirectComposition device to use.
     */
    ZOrderManager(Application& app, Microsoft::WRL::ComPtr<IDCompositionDevice> dcompDevice);
    
    /**
     * @brief Destructor for the ZOrderManager class.
     */
    ~ZOrderManager();
    
    /**
     * @brief Initializes the Z-order manager.
     * @return True if initialization succeeded, false otherwise.
     */
    bool Initialize();
    
    /**
     * @brief Shuts down the Z-order manager and releases resources.
     */
    void Shutdown();
    
    /**
     * @brief Creates a visual element.
     * @param name Name of the visual.
     * @param layerType Layer type for the visual.
     * @param zOrder Z-order value (higher values are on top, used for Custom layer type).
     * @return Visual element or nullptr if creation failed.
     */
    Microsoft::WRL::ComPtr<IDCompositionVisual> CreateVisual(
        const std::string& name,
        LayerType layerType = LayerType::Content,
        int zOrder = 0
    );
    
    /**
     * @brief Adds a visual element to the composition tree.
     * @param name Name of the visual.
     * @param visual Visual element to add.
     * @param layerType Layer type for the visual.
     * @param zOrder Z-order value (higher values are on top, used for Custom layer type).
     * @return True if addition succeeded, false otherwise.
     */
    bool AddVisual(
        const std::string& name,
        Microsoft::WRL::ComPtr<IDCompositionVisual> visual,
        LayerType layerType = LayerType::Content,
        int zOrder = 0
    );
    
    /**
     * @brief Removes a visual element.
     * @param name Name of the visual to remove.
     * @return True if removal succeeded, false otherwise.
     */
    bool RemoveVisual(const std::string& name);
    
    /**
     * @brief Gets a visual element by name.
     * @param name Name of the visual.
     * @return Visual element or nullptr if not found.
     */
    Microsoft::WRL::ComPtr<IDCompositionVisual> GetVisual(const std::string& name);
    
    /**
     * @brief Sets the visibility of a visual element.
     * @param name Name of the visual.
     * @param visible Whether the visual should be visible.
     * @return True if visibility was set, false otherwise.
     */
    bool SetVisualVisibility(const std::string& name, bool visible);
    
    /**
     * @brief Sets the Z-order of a visual element.
     * @param name Name of the visual.
     * @param layerType New layer type for the visual.
     * @param zOrder New Z-order value (higher values are on top, used for Custom layer type).
     * @return True if Z-order was set, false otherwise.
     */
    bool SetVisualZOrder(
        const std::string& name,
        LayerType layerType = LayerType::Content,
        int zOrder = 0
    );
    
    /**
     * @brief Gets the root visual for the composition tree.
     * @return Root visual element.
     */
    Microsoft::WRL::ComPtr<IDCompositionVisual> GetRootVisual() const { return m_rootVisual; }
    
    /**
     * @brief Applies pending changes to the composition.
     * @return True if commit succeeded, false otherwise.
     */
    bool Commit();

private:
    /**
     * @brief Rebuilds the composition tree based on current Z-ordering.
     */
    void RebuildTree();
    
    /**
     * @brief Gets the Z-order value for a layer type.
     * @param layerType Layer type.
     * @return Z-order value.
     */
    int GetLayerBaseZOrder(LayerType layerType) const;
    
    /**
     * @brief Log a message using the application logger.
     * @tparam Args Variadic template for format arguments.
     * @param level The log level (0=trace, 1=debug, 2=info, 3=warning, 4=error, 5=critical).
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    void Log(int level, const std::string& fmt, const Args&... args);

    /**
     * @struct VisualInfo
     * @brief Information about a visual element.
     */
    struct VisualInfo {
        Microsoft::WRL::ComPtr<IDCompositionVisual> visual; ///< Visual element
        LayerType layerType;                               ///< Layer type
        int zOrder;                                        ///< Z-order value
        bool visible;                                      ///< Visibility state
    };

    Application& m_app;                                    ///< Reference to the main application
    Microsoft::WRL::ComPtr<IDCompositionDevice> m_dcompDevice; ///< DirectComposition device
    Microsoft::WRL::ComPtr<IDCompositionVisual> m_rootVisual; ///< Root visual element
    
    std::unordered_map<std::string, VisualInfo> m_visuals; ///< Map of visual elements by name
    bool m_initialized;                                    ///< Whether the manager is initialized
    bool m_treeNeedsRebuild;                              ///< Whether the tree needs rebuilding
};

} // namespace poe