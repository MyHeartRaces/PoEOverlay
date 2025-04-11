#include "core/Settings.h"
#include "core/Application.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace poe {

Settings::Settings(Application& app)
    : m_app(app)
    , m_dirty(false)
{
    // Set default settings file path
    auto appDataPath = std::filesystem::temp_directory_path() / "PoEOverlay";
    m_settingsFilePath = appDataPath / "settings.json";
}

Settings::~Settings()
{
    Shutdown();
}

bool Settings::Initialize()
{
    try {
        // Create directories if they don't exist
        std::filesystem::create_directories(m_settingsFilePath.parent_path());
        
        // Load settings from file or create defaults if file doesn't exist
        if (!Load()) {
            CreateDefaultSettings();
            Save();
        }
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize settings: " << e.what() << std::endl;
        return false;
    }
}

void Settings::Shutdown()
{
    // Save settings if they've been modified
    if (m_dirty) {
        Save();
    }
}

bool Settings::HasSetting(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.find(key) != m_settings.end();
}

bool Settings::RemoveSetting(const std::string& key)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_settings.find(key);
    if (it != m_settings.end()) {
        m_settings.erase(it);
        m_dirty = true;
        return true;
    }
    return false;
}

bool Settings::Save()
{
    try {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Create directory if it doesn't exist
        std::filesystem::create_directories(m_settingsFilePath.parent_path());
        
        // Convert settings to JSON and write to file
        auto json = SettingsToJson();
        std::ofstream file(m_settingsFilePath);
        if (!file.is_open()) {
            return false;
        }
        
        file << json.dump(4); // Pretty print with 4-space indentation
        file.close();
        
        m_dirty = false;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to save settings: " << e.what() << std::endl;
        return false;
    }
}

bool Settings::Load()
{
    try {
        // Check if file exists
        if (!std::filesystem::exists(m_settingsFilePath)) {
            return false;
        }
        
        // Read file into JSON
        std::ifstream file(m_settingsFilePath);
        if (!file.is_open()) {
            return false;
        }
        
        nlohmann::json json;
        file >> json;
        file.close();
        
        // Convert JSON to settings
        std::lock_guard<std::mutex> lock(m_mutex);
        JsonToSettings(json);
        
        m_dirty = false;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load settings: " << e.what() << std::endl;
        return false;
    }
}

void Settings::Reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_settings.clear();
    CreateDefaultSettings();
    m_dirty = true;
}

void Settings::SetSettingsFilePath(const std::filesystem::path& path)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_settingsFilePath = path;
}

const std::filesystem::path& Settings::GetSettingsFilePath() const
{
    return m_settingsFilePath;
}

nlohmann::json Settings::SettingsToJson() const
{
    nlohmann::json json = nlohmann::json::object();
    
    // Convert settings to JSON
    // Note: This is limited to basic types that can be serialized to JSON
    for (const auto& [key, value] : m_settings) {
        if (value.type() == typeid(int)) {
            json[key] = std::any_cast<int>(value);
        }
        else if (value.type() == typeid(double)) {
            json[key] = std::any_cast<double>(value);
        }
        else if (value.type() == typeid(bool)) {
            json[key] = std::any_cast<bool>(value);
        }
        else if (value.type() == typeid(std::string)) {
            json[key] = std::any_cast<std::string>(value);
        }
        // Add more types as needed
    }
    
    return json;
}

void Settings::JsonToSettings(const nlohmann::json& json)
{
    m_settings.clear();
    
    // Convert JSON to settings
    for (auto it = json.begin(); it != json.end(); ++it) {
        const auto& key = it.key();
        const auto& value = it.value();
        
        if (value.is_number_integer()) {
            m_settings[key] = value.get<int>();
        }
        else if (value.is_number_float()) {
            m_settings[key] = value.get<double>();
        }
        else if (value.is_boolean()) {
            m_settings[key] = value.get<bool>();
        }
        else if (value.is_string()) {
            m_settings[key] = value.get<std::string>();
        }
        // Add more types as needed
    }
}

void Settings::CreateDefaultSettings()
{
    // Set default application settings
    Set("window.width", 800);
    Set("window.height", 600);
    Set("window.x", 100);
    Set("window.y", 100);
    Set("window.opacity", 0.9);
    Set("hotkey.toggle", "Alt+B");
    Set("hotkey.interactive", "Alt+I");
    Set("browser.homepage", "https://www.pathofexile.com");
    Set("browser.searchEngine", "https://www.google.com/search?q=");
    Set("browser.historyEnabled", true);
    Set("browser.cookiesEnabled", true);
    Set("performance.suspendWhenHidden", true);
    Set("performance.throttleWhenGameActive", true);
}

} // namespace poe