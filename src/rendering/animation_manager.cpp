#include "rendering/animation_manager.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"

#include <algorithm>

namespace poe {

//-----------------------------------------------------------------------------
// Animation implementation
//-----------------------------------------------------------------------------

Animation::Animation(const std::string& name, uint32_t durationMs)
    : m_name(name)
    , m_durationMs(durationMs)
    , m_running(false)
    , m_progress(0.0f)
{
}

void Animation::Start()
{
    m_startTime = std::chrono::steady_clock::now();
    m_running = true;
    m_progress = 0.0f;
    
    // Update the initial value
    UpdateValue();
}

void Animation::Stop()
{
    m_running = false;
}

bool Animation::Update()
{
    if (!m_running) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime).count();
    
    // Calculate progress
    if (m_durationMs <= 0) {
        m_progress = 1.0f;
    } else {
        m_progress = static_cast<float>(elapsed) / static_cast<float>(m_durationMs);
    }
    
    // Clamp progress to 0.0-1.0
    m_progress = (m_progress < 0.0f) ? 0.0f : (m_progress > 1.0f) ? 1.0f : m_progress;
    
    // Update the animated value
    UpdateValue();
    
    // Check if animation has finished
    if (m_progress >= 1.0f) {
        m_running = false;
        
        // Call the completion callback if set
        if (m_completionCallback) {
            m_completionCallback();
        }
        
        return false;
    }
    
    return true;
}

//-----------------------------------------------------------------------------
// FloatAnimation implementation
//-----------------------------------------------------------------------------

FloatAnimation::FloatAnimation(
    const std::string& name,
    uint32_t durationMs,
    float startValue,
    float endValue,
    std::function<void(float)> valueCallback
)
    : Animation(name, durationMs)
    , m_startValue(startValue)
    , m_endValue(endValue)
    , m_currentValue(startValue)
    , m_valueCallback(valueCallback)
{
}

void FloatAnimation::UpdateValue()
{
    // Calculate the current value based on progress
    m_currentValue = m_startValue + (m_endValue - m_startValue) * m_progress;
    
    // Call the value callback
    if (m_valueCallback) {
        m_valueCallback(m_currentValue);
    }
}

//-----------------------------------------------------------------------------
// AnimationManager implementation
//-----------------------------------------------------------------------------

AnimationManager::AnimationManager(Application& app)
    : m_app(app)
    , m_initialized(false)
{
}

AnimationManager::~AnimationManager()
{
    Shutdown();
}

bool AnimationManager::Initialize()
{
    if (m_initialized) {
        return true;
    }
    
    Log(2, "Animation Manager initialized");
    m_initialized = true;
    return true;
}

void AnimationManager::Shutdown()
{
    if (!m_initialized) {
        return;
    }
    
    StopAllAnimations();
    m_animations.clear();
    m_activeAnimations.clear();
    
    m_initialized = false;
    Log(2, "Animation Manager shutdown");
}

void AnimationManager::Update()
{
    if (!m_initialized) {
        return;
    }
    
    // Update all active animations
    auto it = m_activeAnimations.begin();
    while (it != m_activeAnimations.end()) {
        auto& animation = *it;
        
        if (animation->Update()) {
            // Animation is still running, move to next
            ++it;
        } else {
            // Animation has finished, remove from active list
            it = m_activeAnimations.erase(it);
        }
    }
}

std::shared_ptr<FloatAnimation> AnimationManager::CreateFloatAnimation(
    const std::string& name,
    uint32_t durationMs,
    float startValue,
    float endValue,
    std::function<void(float)> valueCallback
)
{
    if (!m_initialized) {
        Log(3, "Cannot create animation, manager not initialized");
        return nullptr;
    }
    
    // Check if animation with this name already exists
    auto existingAnim = m_animations.find(name);
    if (existingAnim != m_animations.end()) {
        // Stop existing animation
        existingAnim->second->Stop();
        
        // If it's active, remove it from active list
        auto activeIt = std::find(m_activeAnimations.begin(), m_activeAnimations.end(), existingAnim->second);
        if (activeIt != m_activeAnimations.end()) {
            m_activeAnimations.erase(activeIt);
        }
        
        // Remove from animations map
        m_animations.erase(existingAnim);
    }
    
    // Create new animation
    auto animation = std::make_shared<FloatAnimation>(
        name,
        durationMs,
        startValue,
        endValue,
        valueCallback
    );
    
    // Add to animations map
    m_animations[name] = animation;
    
    Log(1, "Created float animation: {}", name);
    return animation;
}

bool AnimationManager::StartAnimation(const std::string& name)
{
    if (!m_initialized) {
        return false;
    }
    
    auto it = m_animations.find(name);
    if (it == m_animations.end()) {
        Log(3, "Cannot start animation, not found: {}", name);
        return false;
    }
    
    auto& animation = it->second;
    
    // Start the animation
    animation->Start();
    
    // Add to active animations if not already there
    auto activeIt = std::find(m_activeAnimations.begin(), m_activeAnimations.end(), animation);
    if (activeIt == m_activeAnimations.end()) {
        m_activeAnimations.push_back(animation);
    }
    
    Log(1, "Started animation: {}", name);
    return true;
}

bool AnimationManager::StopAnimation(const std::string& name)
{
    if (!m_initialized) {
        return false;
    }
    
    auto it = m_animations.find(name);
    if (it == m_animations.end()) {
        return false;
    }
    
    auto& animation = it->second;
    
    // Stop the animation
    animation->Stop();
    
    // Remove from active animations
    auto activeIt = std::find(m_activeAnimations.begin(), m_activeAnimations.end(), animation);
    if (activeIt != m_activeAnimations.end()) {
        m_activeAnimations.erase(activeIt);
    }
    
    return true;
}

void AnimationManager::StopAllAnimations()
{
    if (!m_initialized) {
        return;
    }
    
    // Stop all animations
    for (auto& pair : m_animations) {
        pair.second->Stop();
    }
    
    // Clear active animations
    m_activeAnimations.clear();
}

std::shared_ptr<Animation> AnimationManager::GetAnimation(const std::string& name)
{
    if (!m_initialized) {
        return nullptr;
    }
    
    auto it = m_animations.find(name);
    if (it == m_animations.end()) {
        return nullptr;
    }
    
    return it->second;
}

template<typename... Args>
void AnimationManager::Log(int level, const std::string& fmt, const Args&... args)
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
        std::cerr << "AnimationManager log error: " << e.what() << std::endl;
    }
}

} // namespace poe