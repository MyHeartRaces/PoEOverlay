#pragma once

#include <string>
#include <unordered_map>
#include <any>
#include <mutex>
#include <optional>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace poe {

// Forward declarations
class Application;

/**
 * @class Settings
 * @brief Manages application settings and configuration.
 * 
 * This class provides a thread-safe interface for storing, retrieving, and
 * persisting application settings. It supports various data types and
 * automatic serialization/deserialization to/from JSON.
 */
class Settings {
public:
    /**
     * @brief Constructor for the Settings class.
     * @param app Reference to the main application instance.
     */
    explicit Settings(Application& app);

    /**
     * @brief Destructor for the Settings class.
     */
    ~Settings();

    /**
     * @brief Initializes the settings system, loading configuration from disk.
     * @return True if initialization was successful, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Shuts down the settings system, saving any pending changes.
     */
    void Shutdown();

    /**
     * @brief Gets a setting value of the specified type.
     * @tparam T The type of the setting value.
     * @param key The key identifying the setting.
     * @param defaultValue The default value to return if the key doesn't exist.
     * @return The setting value, or the default value if not found.
     */
    template<typename T>
    T Get(const std::string& key, const T& defaultValue) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_settings.find(key);
        if (it != m_settings.end()) {
            try {
                return std::any_cast<T>(it->second);
            }
            catch (const std::bad_any_cast&) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    /**
     * @brief Sets a setting value.
     * @tparam T The type of the setting value.
     * @param key The key identifying the setting.
     * @param value The value to set.
     */
    template<typename T>
    void Set(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_settings[key] = value;
        m_dirty = true;
    }

    /**
     * @brief Checks if a setting exists.
     * @param key The key identifying the setting.
     * @return True if the setting exists, false otherwise.
     */
    bool HasSetting(const std::string& key) const;

    /**
     * @brief Removes a setting.
     * @param key The key identifying the setting to remove.
     * @return True if the setting was removed, false if it didn't exist.
     */
    bool RemoveSetting(const std::string& key);

    /**
     * @brief Saves the current settings to disk.
     * @return True if the settings were saved successfully, false otherwise.
     */
    bool Save();

    /**
     * @brief Loads settings from disk.
     * @return True if the settings were loaded successfully, false otherwise.
     */
    bool Load();

    /**
     * @brief Resets all settings to default values.
     */
    void Reset();

    /**
     * @brief Sets the path to the settings file.
     * @param path The path to the settings file.
     */
    void SetSettingsFilePath(const std::filesystem::path& path);

    /**
     * @brief Gets the path to the settings file.
     * @return The path to the settings file.
     */
    const std::filesystem::path& GetSettingsFilePath() const;

private:
    /**
     * @brief Converts settings to JSON for serialization.
     * @return JSON object containing all settings.
     */
    nlohmann::json SettingsToJson() const;

    /**
     * @brief Loads settings from JSON after deserialization.
     * @param json JSON object containing settings.
     */
    void JsonToSettings(const nlohmann::json& json);

    /**
     * @brief Creates default settings if none exist.
     */
    void CreateDefaultSettings();

    /**
     * @brief Reference to the main application instance.
     */
    Application& m_app;

    /**
     * @brief Map storing all settings as key-value pairs.
     */
    std::unordered_map<std::string, std::any> m_settings;

    /**
     * @brief Mutex for thread-safe access to settings.
     */
    mutable std::mutex m_mutex;

    /**
     * @brief Flag indicating if settings have been modified since last save.
     */
    bool m_dirty;

    /**
     * @brief Path to the settings file.
     */
    std::filesystem::path m_settingsFilePath;
};

} // namespace poe