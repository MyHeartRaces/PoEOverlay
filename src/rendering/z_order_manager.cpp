#include "rendering/z_order_manager.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"

#include <algorithm>
#include <vector>

namespace poe {

ZOrderManager::ZOrderManager(Application& app, Microsoft::WRL::ComPtr<IDCompositionDevice> dcompDevice)
    : m_app(app)
    , m_dcompDevice(dcompDevice)
    , m_initialized(false)
    , m_treeNeedsRebuild(false)
{
    Log(2, "ZOrderManager created");
}

ZOrderManager::~ZOrderManager()
{
    Shutdown();
}

bool ZOrderManager::Initialize()
{
    if (m_initialized) {
        return true;
    }

    if (!m_dcompDevice) {
        Log(4, "Cannot initialize ZOrderManager: DirectComposition device is null");
        return false;
    }

    try {
        // Create root visual
        HRESULT hr = m_dcompDevice->CreateVisual(&m_rootVisual);
        if (FAILED(hr)) {
            Log(4, "Failed to create root visual: 0x{:X}", hr);
            return false;
        }

        m_initialized = true;
        Log(2, "ZOrderManager initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        m_app.GetErrorHandler().ReportException(e, ErrorSeverity::Error, "ZOrderManager");
        return false;
    }
}

void ZOrderManager::Shutdown()
{
    if (!m_initialized) {
        return;
    }

    // Clear all visuals
    m_visuals.clear();
    m_rootVisual.Reset();

    m_initialized = false;
    Log(2, "ZOrderManager shutdown");
}

Microsoft::WRL::ComPtr<IDCompositionVisual> ZOrderManager::CreateVisual(
    const std::string& name,
    LayerType layerType,
    int zOrder)
{
    if (!m_initialized) {
        Log(4, "Cannot create visual: ZOrderManager not initialized");
        return nullptr;
    }

    try {
        // Check if visual with this name already exists
        auto it = m_visuals.find(name);
        if (it != m_visuals.end()) {
            Log(3, "Visual '{}' already exists, returning existing visual", name);
            return it->second.visual;
        }

        // Create the visual
        Microsoft::WRL::ComPtr<IDCompositionVisual> visual;
        HRESULT hr = m_dcompDevice->CreateVisual(&visual);
        if (FAILED(hr)) {
            Log(4, "Failed to create visual '{}': 0x{:X}", name, hr);
            return nullptr;
        }

        // Add to our map
        VisualInfo info;
        info.visual = visual;
        info.layerType = layerType;
        info.zOrder = zOrder;
        info.visible = true;

        m_visuals[name] = info;
        m_treeNeedsRebuild = true;

        Log(1, "Created visual '{}' with layer type {} and Z-order {}", 
            name, static_cast<int>(layerType), zOrder);

        return visual;
    }
    catch (const std::exception& e) {
        m_app.GetErrorHandler().ReportException(e, ErrorSeverity::Error, "ZOrderManager");
        return nullptr;
    }
}

bool ZOrderManager::AddVisual(
    const std::string& name,
    Microsoft::WRL::ComPtr<IDCompositionVisual> visual,
    LayerType layerType,
    int zOrder)
{
    if (!m_initialized) {
        Log(4, "Cannot add visual: ZOrderManager not initialized");
        return false;
    }

    if (!visual) {
        Log(4, "Cannot add visual '{}': Visual is null", name);
        return false;
    }

    try {
        // Check if visual with this name already exists
        auto it = m_visuals.find(name);
        if (it != m_visuals.end()) {
            Log(3, "Visual '{}' already exists, replacing", name);
            it->second.visual = visual;
            it->second.layerType = layerType;
            it->second.zOrder = zOrder;
        } else {
            // Add to our map
            VisualInfo info;
            info.visual = visual;
            info.layerType = layerType;
            info.zOrder = zOrder;
            info.visible = true;

            m_visuals[name] = info;
        }

        m_treeNeedsRebuild = true;

        Log(1, "Added visual '{}' with layer type {} and Z-order {}", 
            name, static_cast<int>(layerType), zOrder);

        return true;
    }
    catch (const std::exception& e) {
        m_app.GetErrorHandler().ReportException(e, ErrorSeverity::Error, "ZOrderManager");
        return false;
    }
}

bool ZOrderManager::RemoveVisual(const std::string& name)
{
    if (!m_initialized) {
        return false;
    }

    auto it = m_visuals.find(name);
    if (it == m_visuals.end()) {
        Log(3, "Cannot remove visual '{}': Not found", name);
        return false;
    }

    m_visuals.erase(it);
    m_treeNeedsRebuild = true;

    Log(1, "Removed visual '{}'", name);
    return true;
}

Microsoft::WRL::ComPtr<IDCompositionVisual> ZOrderManager::GetVisual(const std::string& name)
{
    if (!m_initialized) {
        return nullptr;
    }

    auto it = m_visuals.find(name);
    if (it == m_visuals.end()) {
        return nullptr;
    }

    return it->second.visual;
}

bool ZOrderManager::SetVisualVisibility(const std::string& name, bool visible)
{
    if (!m_initialized) {
        return false;
    }

    auto it = m_visuals.find(name);
    if (it == m_visuals.end()) {
        Log(3, "Cannot set visibility for visual '{}': Not found", name);
        return false;
    }

    if (it->second.visible != visible) {
        it->second.visible = visible;
        it->second.visual->SetOpacity(visible ? 1.0f : 0.0f);
        m_treeNeedsRebuild = true;

        Log(1, "Set visibility of visual '{}' to {}", name, visible ? "visible" : "hidden");
    }

    return true;
}

bool ZOrderManager::SetVisualZOrder(
    const std::string& name,
    LayerType layerType,
    int zOrder)
{
    if (!m_initialized) {
        return false;
    }

    auto it = m_visuals.find(name);
    if (it == m_visuals.end()) {
        Log(3, "Cannot set Z-order for visual '{}': Not found", name);
        return false;
    }

    if (it->second.layerType != layerType || it->second.zOrder != zOrder) {
        it->second.layerType = layerType;
        it->second.zOrder = zOrder;
        m_treeNeedsRebuild = true;

        Log(1, "Set Z-order of visual '{}' to layer type {} and Z-order {}", 
            name, static_cast<int>(layerType), zOrder);
    }

    return true;
}

bool ZOrderManager::Commit()
{
    if (!m_initialized) {
        return false;
    }

    try {
        // Rebuild the composition tree if needed
        if (m_treeNeedsRebuild) {
            RebuildTree();
            m_treeNeedsRebuild = false;
        }

        // Commit changes
        HRESULT hr = m_dcompDevice->Commit();
        if (FAILED(hr)) {
            Log(4, "Failed to commit composition changes: 0x{:X}", hr);
            return false;
        }

        return true;
    }
    catch (const std::exception& e) {
        m_app.GetErrorHandler().ReportException(e, ErrorSeverity::Error, "ZOrderManager");
        return false;
    }
}

void ZOrderManager::RebuildTree()
{
    if (!m_initialized || !m_rootVisual) {
        return;
    }

    try {
        // Remove all visuals from the root
        m_rootVisual->RemoveAllVisuals();

        // Sort visuals by layer type and Z-order
        std::vector<std::pair<std::string, VisualInfo*>> sortedVisuals;
        for (auto& pair : m_visuals) {
            if (pair.second.visible) {
                sortedVisuals.push_back({ pair.first, &pair.second });
            }
        }

        std::sort(sortedVisuals.begin(), sortedVisuals.end(), 
            [this](const auto& a, const auto& b) {
                int aBase = GetLayerBaseZOrder(a.second->layerType);
                int bBase = GetLayerBaseZOrder(b.second->layerType);
                
                if (aBase != bBase) {
                    return aBase < bBase; // Sort by layer type first
                }
                
                return a.second->zOrder < b.second->zOrder; // Then by Z-order within layer
            }
        );

        // Add visuals to the root in order (lower Z-order first)
        for (const auto& pair : sortedVisuals) {
            m_rootVisual->AddVisual(pair.second->visual.Get(), FALSE, nullptr);
        }

        Log(1, "Rebuilt composition tree with {} visible visuals", sortedVisuals.size());
    }
    catch (const std::exception& e) {
        m_app.GetErrorHandler().ReportException(e, ErrorSeverity::Error, "ZOrderManager");
    }
}

int ZOrderManager::GetLayerBaseZOrder(LayerType layerType) const
{
    switch (layerType) {
        case LayerType::Background: return 0;
        case LayerType::Content:    return 1000;
        case LayerType::UI:         return 2000;
        case LayerType::Popup:      return 3000;
        case LayerType::Border:     return 4000;
        case LayerType::Foreground: return 5000;
        case LayerType::Custom:     return 10000;
        default:                    return 0;
    }
}

template<typename... Args>
void ZOrderManager::Log(int level, const std::string& fmt, const Args&... args)
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
        std::cerr << "ZOrderManager log error: " << e.what() << std::endl;
    }
}

} // namespace poe