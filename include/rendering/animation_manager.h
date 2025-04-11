#pragma once

#include <Windows.h>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "core/Application.h"

namespace poe {

/**
 * @class Animation
 * @brief Base class for animations.
 */
class Animation {
public:
    /**
     * @brief Constructor for the Animation class.
     * @param name The name of the animation.
     * @param durationMs The duration of the animation in milliseconds.
     */
    Animation(const std::string& name, uint32_t durationMs);
    
    /**
     * @brief Virtual destructor for proper polymorphic destruction.
     */
    virtual ~Animation() = default;
    
    /**
     * @brief Starts the animation.
     */
    void Start();
    
    /**
     * @brief Stops the animation.
     */
    void Stop();
    
    /**
     * @brief Updates the animation.
     * @return True if the animation is still running, false if it has finished.
     */
    bool Update();
    
    /**
     * @brief Gets the name of the animation.
     * @return The animation name.
     */
    const std::string& GetName() const { return m_name; }
    
    /**
     * @brief Checks if the animation is running.
     * @return True if the animation is running, false otherwise.
     */
    bool IsRunning() const { return m_running; }
    
    /**
     * @brief Gets the current progress of the animation (0.0-1.0).
     * @return The animation progress.
     */
    float GetProgress() const { return m_progress; }
    
    /**
     * @brief Sets a callback function to call when the animation completes.
     * @param callback The callback function.
     */
    void SetCompletionCallback(std::function<void()> callback) { m_completionCallback = callback; }

protected:
    /**
     * @brief Updates the animation value based on the current progress.
     * Must be implemented by derived classes.
     */
    virtual void UpdateValue() = 0;

    std::string m_name;                        ///< Animation name
    uint32_t m_durationMs;                     ///< Animation duration in milliseconds
    bool m_running;                            ///< Whether the animation is running
    float m_progress;                          ///< Current animation progress (0.0-1.0)
    std::chrono::steady_clock::time_point m_startTime; ///< Animation start time
    std::function<void()> m_completionCallback; ///< Callback for animation completion
};

/**
 * @class FloatAnimation
 * @brief Animation for float values.
 */
class FloatAnimation : public Animation {
public:
    /**
     * @brief Constructor for the FloatAnimation class.
     * @param name The name of the animation.
     * @param durationMs The duration of the animation in milliseconds.
     * @param startValue The starting value.
     * @param endValue The ending value.
     * @param valueCallback The callback to call with the updated value.
     */
    FloatAnimation(
        const std::string& name,
        uint32_t durationMs,
        float startValue,
        float endValue,
        std::function<void(float)> valueCallback
    );

protected:
    /**
     * @brief Updates the animated value and calls the callback.
     */
    void UpdateValue() override;

private:
    float m_startValue;                       ///< Starting value
    float m_endValue;                         ///< Ending value
    float m_currentValue;                     ///< Current value
    std::function<void(float)> m_valueCallback; ///< Callback for value updates
};

/**
 * @class AnimationManager
 * @brief Manages animations for the application.
 */
class AnimationManager {
public:
    /**
     * @brief Constructor for the AnimationManager class.
     * @param app Reference to the main application instance.
     */
    explicit AnimationManager(Application& app);
    
    /**
     * @brief Destructor for the AnimationManager class.
     */
    ~AnimationManager();
    
    /**
     * @brief Initializes the animation manager.
     * @return True if initialization was successful, false otherwise.
     */
    bool Initialize();
    
    /**
     * @brief Shuts down the animation manager.
     */
    void Shutdown();
    
    /**
     * @brief Updates all running animations.
     */
    void Update();
    
    /**
     * @brief Creates a float animation.
     * @param name The name of the animation.
     * @param durationMs The duration of the animation in milliseconds.
     * @param startValue The starting value.
     * @param endValue The ending value.
     * @param valueCallback The callback to call with the updated value.
     * @return Shared pointer to the created animation.
     */
    std::shared_ptr<FloatAnimation> CreateFloatAnimation(
        const std::string& name,
        uint32_t durationMs,
        float startValue,
        float endValue,
        std::function<void(float)> valueCallback
    );
    
    /**
     * @brief Starts an animation by name.
     * @param name The name of the animation.
     * @return True if the animation was found and started, false otherwise.
     */
    bool StartAnimation(const std::string& name);
    
    /**
     * @brief Stops an animation by name.
     * @param name The name of the animation.
     * @return True if the animation was found and stopped, false otherwise.
     */
    bool StopAnimation(const std::string& name);
    
    /**
     * @brief Stops all animations.
     */
    void StopAllAnimations();
    
    /**
     * @brief Gets an animation by name.
     * @param name The name of the animation.
     * @return Shared pointer to the animation, or nullptr if not found.
     */
    std::shared_ptr<Animation> GetAnimation(const std::string& name);

private:
    /**
     * @brief Log a message using the application logger.
     * @tparam Args Variadic template for format arguments.
     * @param level The log level (0=trace, 1=debug, 2=info, 3=warning, 4=error, 5=critical).
     * @param fmt Format string.
     * @param args Format arguments.
     */
    template<typename... Args>
    void Log(int level, const std::string& fmt, const Args&... args);

    Application& m_app;                       ///< Reference to the main application
    bool m_initialized;                       ///< Whether the manager is initialized
    std::unordered_map<std::string, std::shared_ptr<Animation>> m_animations; ///< Registered animations
    std::vector<std::shared_ptr<Animation>> m_activeAnimations; ///< Currently active animations
};

} // namespace poe