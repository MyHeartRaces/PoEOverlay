#include "browser/BrowserInterface.h"
#include "browser/CefManager.h"
#include "browser/BrowserView.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"
#include "core/Settings.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace poe {

BrowserInterface::BrowserInterface(Application& app)
    : m_app(app)
    , m_homePage("poe://home")
    , m_newTabPage("poe://home")
    , m_searchEngine("https://www.google.com/search?q={}")
{
    Log(2, "BrowserInterface created");
}

BrowserInterface::~BrowserInterface()
{
    Shutdown();
}

bool BrowserInterface::Initialize()
{
    try
    {
        Log(2, "Initializing BrowserInterface");
        
        // Initialize CEF
        CefManager::CefConfig cefConfig;
        
        // Set paths for CEF
        auto appDataPath = std::filesystem::temp_directory_path() / "PoEOverlay";
        cefConfig.cachePath = (appDataPath / "cef" / "cache").string();
        cefConfig.userDataPath = (appDataPath / "cef" / "user_data").string();
        
        // Get CEF resources path from settings
        std::string resourcesPath = m_app.GetSettings().Get<std::string>("cef.resourcesPath", "Resources");
        cefConfig.resourcesPath = resourcesPath;
        
        // Use default locales path
        cefConfig.localesPath = (std::filesystem::path(resourcesPath) / "locales").string();
        
        // Enable offscreen rendering
        cefConfig.enableOffscreenRendering = true;
        
        // Disable persistent session cookies
        cefConfig.persistSessionCookies = m_app.GetSettings().Get<bool>("browser.persistCookies", true);
        
        // Background process priority
        cefConfig.backgroundProcessPriority = m_app.GetSettings().Get<int>("browser.backgroundPriority", 0);
        
        // Create CEF manager
        m_cefManager = std::make_unique<CefManager>(m_app, cefConfig);
        
        // Initialize CEF
        if (!m_cefManager->Initialize())
        {
            Log(4, "Failed to initialize CEF");
            return false;
        }
        
        // Load home page and search settings from settings
        m_homePage = m_app.GetSettings().Get<std::string>("browser.homePage", "poe://home");
        m_newTabPage = m_app.GetSettings().Get<std::string>("browser.newTabPage", "poe://home");
        m_searchEngine = m_app.GetSettings().Get<std::string>("browser.searchEngine", "https://www.google.com/search?q={}");
        
        // Load bookmarks
        LoadBookmarks();
        
        Log(2, "BrowserInterface initialized successfully");
        return true;
    }
    catch (const std::exception& ex)
    {
        m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "BrowserInterface");
        return false;
    }
}

void BrowserInterface::Shutdown()
{
    // Save bookmarks
    SaveBookmarks();
    
    // Close all browser views
    m_browserViews.clear();
    
    // Shutdown CEF
    if (m_cefManager)
    {
        m_cefManager->Shutdown();
        m_cefManager.reset();
    }
    
    Log(2, "BrowserInterface shutdown");
}

std::shared_ptr<BrowserView> BrowserInterface::CreateBrowserView(
    int width,
    int height,
    const std::string& url)
{
    if (!m_cefManager)
    {
        Log(4, "Cannot create browser view: CEF not initialized");
        return nullptr;
    }

    try
    {
        Log(2, "Creating browser view: {}x{}, URL: {}", width, height, url);
        
        // Create browser view
        auto browserView = std::make_shared<BrowserView>(
            m_app,
            *m_cefManager,
            width,
            height,
            url
        );
        
        // Initialize browser view
        if (!browserView->Initialize())
        {
            Log(4, "Failed to initialize browser view");
            return nullptr;
        }
        
        // Add to list of browser views
        m_browserViews.push_back(browserView);
        
        return browserView;
    }
    catch (const std::exception& ex)
    {
        m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "BrowserInterface");
        return nullptr;
    }
}

void BrowserInterface::Update()
{
    if (!m_cefManager)
    {
        return;
    }

    // Process CEF message loop
    m_cefManager->ProcessEvents();
    
    // Remove closed browser views
    m_browserViews.erase(
        std::remove_if(m_browserViews.begin(), m_browserViews.end(),
            [](const std::shared_ptr<BrowserView>& view) {
                return view == nullptr;
            }),
        m_browserViews.end()
    );
}

bool BrowserInterface::AddBookmark(const Bookmark& bookmark)
{
    // Check if URL already exists
    auto it = std::find_if(m_bookmarks.begin(), m_bookmarks.end(),
        [&bookmark](const Bookmark& b) {
            return b.url == bookmark.url;
        });
    
    if (it != m_bookmarks.end())
    {
        // Update existing bookmark
        it->name = bookmark.name;
        it->folder = bookmark.folder;
        it->icon = bookmark.icon;
    }
    else
    {
        // Add new bookmark
        m_bookmarks.push_back(bookmark);
    }
    
    // Save bookmarks
    SaveBookmarks();
    
    return true;
}

bool BrowserInterface::RemoveBookmark(const std::string& url)
{
    auto it = std::find_if(m_bookmarks.begin(), m_bookmarks.end(),
        [&url](const Bookmark& b) {
            return b.url == url;
        });
    
    if (it == m_bookmarks.end())
    {
        return false;
    }
    
    // Remove bookmark
    m_bookmarks.erase(it);
    
    // Save bookmarks
    SaveBookmarks();
    
    return true;
}

std::vector<Bookmark> BrowserInterface::GetBookmarks() const
{
    return m_bookmarks;
}

std::vector<Bookmark> BrowserInterface::GetBookmarksInFolder(const std::string& folder) const
{
    std::vector<Bookmark> result;
    
    for (const auto& bookmark : m_bookmarks)
    {
        if (bookmark.folder == folder)
        {
            result.push_back(bookmark);
        }
    }
    
    return result;
}

std::vector<std::string> BrowserInterface::GetBookmarkFolders() const
{
    std::vector<std::string> folders;
    
    for (const auto& bookmark : m_bookmarks)
    {
        // Check if folder already exists in the list
        if (std::find(folders.begin(), folders.end(), bookmark.folder) == folders.end())
        {
            folders.push_back(bookmark.folder);
        }
    }
    
    return folders;
}

bool BrowserInterface::IsBookmarked(const std::string& url) const
{
    return std::find_if(m_bookmarks.begin(), m_bookmarks.end(),
        [&url](const Bookmark& b) {
            return b.url == url;
        }) != m_bookmarks.end();
}

std::string BrowserInterface::GetHomePage() const
{
    return m_homePage;
}

void BrowserInterface::SetHomePage(const std::string& url)
{
    m_homePage = url;
    
    // Save to settings
    m_app.GetSettings().Set("browser.homePage", url);
}

std::string BrowserInterface::GetNewTabPage() const
{
    return m_newTabPage;
}

void BrowserInterface::SetNewTabPage(const std::string& url)
{
    m_newTabPage = url;
    
    // Save to settings
    m_app.GetSettings().Set("browser.newTabPage", url);
}

std::string BrowserInterface::GetSearchUrl(const std::string& query) const
{
    // Replace placeholder with URL-encoded query
    std::string result = m_searchEngine;
    
    size_t pos = result.find("{}");
    if (pos != std::string::npos)
    {
        // URL encode the query
        std::string encodedQuery;
        for (char c : query)
        {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            {
                encodedQuery += c;
            }
            else if (c == ' ')
            {
                encodedQuery += '+';
            }
            else
            {
                encodedQuery += '%';
                encodedQuery += "0123456789ABCDEF"[(c & 0xF0) >> 4];
                encodedQuery += "0123456789ABCDEF"[c & 0x0F];
            }
        }
        
        result.replace(pos, 2, encodedQuery);
    }
    
    return result;
}

void BrowserInterface::SetSearchEngine(const std::string& url)
{
    m_searchEngine = url;
    
    // Save to settings
    m_app.GetSettings().Set("browser.searchEngine", url);
}

void BrowserInterface::LoadBookmarks()
{
    try
    {
        // Clear existing bookmarks
        m_bookmarks.clear();
        
        // Get bookmarks file path
        auto appDataPath = std::filesystem::temp_directory_path() / "PoEOverlay";
        auto bookmarksPath = appDataPath / "bookmarks.json";
        
        // Check if file exists
        if (!std::filesystem::exists(bookmarksPath))
        {
            // Create default bookmarks
            m_bookmarks = {
                {"Path of Exile", "https://www.pathofexile.com", "Official", ""},
                {"POE Wiki", "https://www.poewiki.net", "Official", ""},
                {"POE Trade", "https://www.pathofexile.com/trade", "Official", ""},
                {"POE Ninja", "https://poe.ninja", "Tools", ""},
                {"POE DB", "https://poedb.tw", "Tools", ""},
                {"Craft of Exile", "https://www.craftofexile.com", "Tools", ""}
            };
            
            // Save default bookmarks
            SaveBookmarks();
            return;
        }
        
        // Open file
        std::ifstream file(bookmarksPath);
        if (!file.is_open())
        {
            Log(3, "Failed to open bookmarks file: {}", bookmarksPath.string());
            return;
        }
        
        // Parse JSON
        nlohmann::json json;
        file >> json;
        
        // Load bookmarks from JSON
        for (const auto& item : json)
        {
            Bookmark bookmark;
            bookmark.name = item["name"].get<std::string>();
            bookmark.url = item["url"].get<std::string>();
            bookmark.folder = item["folder"].get<std::string>();
            
            // Icon is optional
            if (item.contains("icon"))
            {
                bookmark.icon = item["icon"].get<std::string>();
            }
            
            m_bookmarks.push_back(bookmark);
        }
        
        Log(2, "Loaded {} bookmarks", m_bookmarks.size());
    }
    catch (const std::exception& ex)
    {
        m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "BrowserInterface");
    }
}

void BrowserInterface::SaveBookmarks()
{
    try
    {
        // Get bookmarks file path
        auto appDataPath = std::filesystem::temp_directory_path() / "PoEOverlay";
        auto bookmarksPath = appDataPath / "bookmarks.json";
        
        // Create directory if it doesn't exist
        std::filesystem::create_directories(appDataPath);
        
        // Create JSON array
        nlohmann::json json = nlohmann::json::array();
        
        // Add bookmarks to JSON
        for (const auto& bookmark : m_bookmarks)
        {
            nlohmann::json item;
            item["name"] = bookmark.name;
            item["url"] = bookmark.url;
            item["folder"] = bookmark.folder;
            
            // Only include icon if it's not empty
            if (!bookmark.icon.empty())
            {
                item["icon"] = bookmark.icon;
            }
            
            json.push_back(item);
        }
        
        // Open file
        std::ofstream file(bookmarksPath);
        if (!file.is_open())
        {
            Log(3, "Failed to open bookmarks file for writing: {}", bookmarksPath.string());
            return;
        }
        
        // Write JSON to file
        file << json.dump(4);
        
        Log(2, "Saved {} bookmarks", m_bookmarks.size());
    }
    catch (const std::exception& ex)
    {
        m_app.GetErrorHandler().ReportException(ex, ErrorSeverity::Error, "BrowserInterface");
    }
}

template<typename... Args>
void BrowserInterface::Log(int level, const std::string& fmt, const Args&... args)
{
    try
    {
        auto& logger = m_app.GetLogger();
        
        switch (level)
        {
            case 0: logger.Trace(fmt, args...); break;
            case 1: logger.Debug(fmt, args...); break;
            case 2: logger.Info(fmt, args...); break;
            case 3: logger.Warning(fmt, args...); break;
            case 4: logger.Error(fmt, args...); break;
            case 5: logger.Critical(fmt, args...); break;
            default: logger.Info(fmt, args...); break;
        }
    }
    catch (const std::exception& e)
    {
        // Fallback to stderr if logger is unavailable
        std::cerr << "BrowserInterface log error: " << e.what() << std::endl;
    }
}

} // namespace poe