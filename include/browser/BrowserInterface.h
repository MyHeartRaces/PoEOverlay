#pragma once

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include "core/Application.h"

namespace poe {

// Forward declarations
class CefManager;
class BrowserView;

/**
 * @struct Bookmark
 * @brief Represents a browser bookmark.
 */
struct Bookmark {
    std::string name;      ///< Bookmark name
    std::string url;       ///< Bookmark URL
    std::string folder;    ///< Folder containing the bookmark
    std::string icon;      ///< Icon URL or path
};

/**
 * @class BrowserInterface
 * @brief High-level interface for browser functionality.
 * 
 * This class provides a simplified interface for browser operations,
 * abstracting away the details of CEF integration and providing
 * application-specific functionality like bookmark management.
 */
class BrowserInterface {
public:
    /**
     * @brief Constructor for the BrowserInterface class.
     * @param app Reference to the main application instance.
     */
    explicit BrowserInterface(Application& app);

    /**
     * @brief Destructor for the BrowserInterface class.
     */
    ~BrowserInterface();

    /**
     * @brief Initializes the browser interface.
     * @return True if initialization succeeded, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Shuts down the browser interface.
     */
    void Shutdown();

    /**
     * @brief Creates a new browser view.
     * @param width Initial width of the browser view.
     * @param height Initial height of the browser view.
     * @param url Initial URL to navigate to.
     * @return Pointer to the created browser view, or nullptr on failure.
     */
    std::shared_ptr<BrowserView> CreateBrowserView(
        int width = 800,
        int height = 600,
        const std::string& url = "about:blank"
    );

    /**
     * @brief Updates the browser interface.
     * This should be called periodically to process browser events.
     */
    void Update();

    /**
     * @brief Adds a bookmark.
     * @param bookmark The bookmark to add.
     * @return True if the bookmark was added, false otherwise.
     */
    bool AddBookmark(const Bookmark& bookmark);

    /**
     * @brief Removes a bookmark.
     * @param url The URL of the bookmark to remove.
     * @return True if the bookmark was removed, false otherwise.
     */
    bool RemoveBookmark(const std::string& url);

    /**
     * @brief Gets all bookmarks.
     * @return Vector of all bookmarks.
     */
    std::vector<Bookmark> GetBookmarks() const;

    /**
     * @brief Gets bookmarks in a specific folder.
     * @param folder The folder to get bookmarks from.
     * @return Vector of bookmarks in the specified folder.
     */
    std::vector<Bookmark> GetBookmarksInFolder(const std::string& folder) const;

    /**
     * @brief Gets all bookmark folders.
     * @return Vector of folder names.
     */
    std::vector<std::string> GetBookmarkFolders() const;

    /**
     * @brief Checks if a URL is bookmarked.
     * @param url The URL to check.
     * @return True if the URL is bookmarked, false otherwise.
     */
    bool IsBookmarked(const std::string& url) const;

    /**
     * @brief Gets the URL for the home page.
     * @return The home page URL.
     */
    std::string GetHomePage() const;

    /**
     * @brief Sets the URL for the home page.
     * @param url The home page URL.
     */
    void SetHomePage(const std::string& url);

    /**
     * @brief Gets the URL for the new tab page.
     * @return The new tab page URL.
     */
    std::string GetNewTabPage() const;

    /**
     * @brief Sets the URL for the new tab page.
     * @param url The new tab page URL.
     */
    void SetNewTabPage(const std::string& url);

    /**
     * @brief Gets the URL for search queries.
     * @param query The search query.
     * @return The search URL with the query.
     */
    std::string GetSearchUrl(const std::string& query) const;

    /**
     * @brief Sets the search engine URL.
     * @param url The search engine URL (with {} placeholder for query).
     */
    void SetSearchEngine(const std::string& url);

private:
    /**
     * @brief Loads bookmarks from storage.
     */
    void LoadBookmarks();

    /**
     * @brief Saves bookmarks to storage.
     */
    void SaveBookmarks();

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
    
    std::unique_ptr<CefManager> m_cefManager;     ///< CEF manager instance
    std::vector<std::shared_ptr<BrowserView>> m_browserViews; ///< Active browser views
    
    std::vector<Bookmark> m_bookmarks;            ///< List of bookmarks
    std::string m_homePage;                       ///< Home page URL
    std::string m_newTabPage;                     ///< New tab page URL
    std::string m_searchEngine;                   ///< Search engine URL pattern
};

} // namespace poe