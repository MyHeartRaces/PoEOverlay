#include "browser/ResourceHandler.h"
#include "browser/CefManager.h"
#include "core/Logger.h"
#include "core/ErrorHandler.h"

#include <iostream>
#include <filesystem>

namespace poe {

ResourceHandler::ResourceHandler(Application& app, CefManager& cefManager)
    : m_app(app)
    , m_cefManager(cefManager)
    , m_requestComplete(false)
{
    Log(2, "ResourceHandler created");
}

bool ResourceHandler::RegisterCustomScheme(const std::string& scheme)
{
    if (scheme.empty())
    {
        return false;
    }

    Log(2, "Registering custom scheme: {}", scheme);
    m_registeredSchemes[scheme] = true;
    return true;
}

bool ResourceHandler::Open(
    CefRefPtr<CefRequest> request,
    bool& handle_request,
    CefRefPtr<CefCallback> callback)
{
    // This method is called in CefRequestContextHandler::GetResourceHandler
    // Process the request immediately
    return ProcessRequest(request, callback);
}

bool ResourceHandler::ProcessRequest(
    CefRefPtr<CefRequest> request,
    CefRefPtr<CefCallback> callback)
{
    std::string url = request->GetURL().ToString();
    Log(1, "Processing resource request: {}", url);
    
    // Parse URL to get scheme and path
    size_t schemeDelimiter = url.find("://");
    if (schemeDelimiter == std::string::npos)
    {
        Log(3, "Invalid URL format: {}", url);
        return false;
    }
    
    std::string scheme = url.substr(0, schemeDelimiter);
    std::string path = url.substr(schemeDelimiter + 3);
    
    // Check if we handle this scheme
    if (m_registeredSchemes.find(scheme) != m_registeredSchemes.end())
    {
        // Handle custom scheme
        if (HandleCustomScheme(request, scheme, path))
        {
            callback->Continue();
            return true;
        }
    }
    
    // We don't handle this request
    return false;
}

void ResourceHandler::GetResponseHeaders(
    CefRefPtr<CefResponse> response,
    int64& response_length,
    CefString& redirectUrl)
{
    if (!m_resourceData)
    {
        response->SetStatus(404);
        response->SetStatusText("Not Found");
        response_length = 0;
        return;
    }
    
    response->SetStatus(200);
    response->SetStatusText("OK");
    response->SetMimeType(m_resourceData->mimeType);
    response_length = static_cast<int64>(m_resourceData->data.size());
}

bool ResourceHandler::Skip(
    int64 bytes_to_skip,
    int64& bytes_skipped,
    CefRefPtr<CefResourceSkipCallback> callback)
{
    if (!m_resourceData)
    {
        bytes_skipped = 0;
        return false;
    }
    
    // Skip bytes in the data
    size_t skipAmount = static_cast<size_t>(bytes_to_skip);
    if (skipAmount > m_resourceData->data.size() - m_resourceData->offset)
    {
        skipAmount = m_resourceData->data.size() - m_resourceData->offset;
    }
    
    m_resourceData->offset += skipAmount;
    bytes_skipped = static_cast<int64>(skipAmount);
    
    return true;
}

bool ResourceHandler::Read(
    void* data_out,
    int bytes_to_read,
    int& bytes_read,
    CefRefPtr<CefResourceReadCallback> callback)
{
    if (!m_resourceData || m_resourceData->offset >= m_resourceData->data.size())
    {
        bytes_read = 0;
        m_requestComplete = true;
        return false;  // No more data
    }
    
    // Read data
    size_t available = m_resourceData->data.size() - m_resourceData->offset;
    size_t readAmount = std::min(static_cast<size_t>(bytes_to_read), available);
    
    memcpy(data_out, m_resourceData->data.data() + m_resourceData->offset, readAmount);
    m_resourceData->offset += readAmount;
    bytes_read = static_cast<int>(readAmount);
    
    // Check if we're done
    m_requestComplete = (m_resourceData->offset >= m_resourceData->data.size());
    
    return true;
}

bool ResourceHandler::ReadResponse(
    void* data_out,
    int bytes_to_read,
    int& bytes_read,
    CefRefPtr<CefCallback> callback)
{
    // CEF may call ReadResponse instead of Read for compatibility
    bytes_read = 0;
    return false;
}

void ResourceHandler::Cancel()
{
    Log(2, "Resource request cancelled");
    m_resourceData.reset();
    m_requestComplete = true;
}

bool ResourceHandler::HandleCustomScheme(
    CefRefPtr<CefRequest> request,
    const std::string& scheme,
    const std::string& path)
{
    Log(1, "Handling custom scheme: {}://{}", scheme, path);
    
    // Handle "poe" scheme
    if (scheme == "poe")
    {
        // Extract path components
        std::string mainPath = path;
        std::string queryParams;
        size_t queryPos = path.find('?');
        if (queryPos != std::string::npos) {
            mainPath = path.substr(0, queryPos);
            queryParams = path.substr(queryPos + 1);
        }
        
        // Handle special poe:// URLs
        if (mainPath == "home" || mainPath == "")
        {
            // Create resource data for home page
            m_resourceData = std::make_unique<ResourceData>();
            m_resourceData->mimeType = "text/html";
            m_resourceData->data = R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <title>PoEOverlay Home</title>
                    <style>
                        body {
                            font-family: Arial, sans-serif;
                            background-color: #2c3e50;
                            color: #ecf0f1;
                            margin: 0;
                            padding: 20px;
                        }
                        h1 {
                            color: #e74c3c;
                        }
                        .card {
                            background-color: #34495e;
                            border-radius: 5px;
                            padding: 15px;
                            margin-bottom: 20px;
                            box-shadow: 0 4px 8px rgba(0,0,0,0.2);
                        }
                        a {
                            color: #3498db;
                            text-decoration: none;
                        }
                        a:hover {
                            text-decoration: underline;
                        }
                        .quick-links {
                            display: grid;
                            grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
                            gap: 10px;
                        }
                        .quick-link {
                            padding: 10px;
                            background-color: #2c3e50;
                            border-radius: 4px;
                            transition: background-color 0.2s;
                        }
                        .quick-link:hover {
                            background-color: #243342;
                        }
                    </style>
                </head>
                <body>
                    <h1>PoEOverlay</h1>
                    <div class="card">
                        <h2>Quick Links</h2>
                        <div class="quick-links">
                            <a href="https://www.pathofexile.com" class="quick-link">Official Website</a>
                            <a href="https://www.pathofexile.com/trade" class="quick-link">Trade Site</a>
                            <a href="https://www.pathofexile.com/forum" class="quick-link">Forums</a>
                            <a href="https://poe.ninja" class="quick-link">poe.ninja</a>
                            <a href="https://poedb.tw" class="quick-link">PoEDB</a>
                            <a href="https://www.poewiki.net" class="quick-link">Wiki</a>
                            <a href="https://www.craftofexile.com" class="quick-link">Craft of Exile</a>
                            <a href="poe://bookmarks" class="quick-link">Bookmarks</a>
                            <a href="poe://settings" class="quick-link">Settings</a>
                        </div>
                    </div>
                    <div class="card">
                        <h2>Recent Builds</h2>
                        <div id="recent-builds">
                            <p>Your recent build bookmarks will appear here.</p>
                        </div>
                    </div>
                    <div class="card">
                        <h2>Current League Info</h2>
                        <div id="league-info">
                            <p>Loading league information...</p>
                        </div>
                    </div>
                    <script>
                        // Simple script to populate the recent builds from local storage
                        document.addEventListener('DOMContentLoaded', function() {
                            // This would normally be populated from the application's bookmarks
                            const recentBuildsEl = document.getElementById('recent-builds');
                            
                            // Example data - in a real implementation this would come from the app
                            const recentBuilds = [
                                { name: "Toxic Rain Pathfinder", url: "https://www.pathofexile.com/forum/view-thread/2866127" },
                                { name: "Cyclone Slayer", url: "https://www.pathofexile.com/forum/view-thread/2839382" }
                            ];
                            
                            if (recentBuilds.length > 0) {
                                recentBuildsEl.innerHTML = '';
                                const ul = document.createElement('ul');
                                recentBuilds.forEach(build => {
                                    const li = document.createElement('li');
                                    const a = document.createElement('a');
                                    a.href = build.url;
                                    a.textContent = build.name;
                                    li.appendChild(a);
                                    ul.appendChild(li);
                                });
                                recentBuildsEl.appendChild(ul);
                            }
                            
                            // Simulate fetching league info
                            setTimeout(() => {
                                document.getElementById('league-info').innerHTML = `
                                    <p><strong>Current League:</strong> Affliction</p>
                                    <p><strong>End Date:</strong> April 30, 2025</p>
                                    <p><a href="https://www.pathofexile.com/forum/view-forum/leagues">League Details</a></p>
                                `;
                            }, 500);
                        });
                    </script>
                </body>
                </html>
            )";
            m_resourceData->offset = 0;
            
            return true;
        }
        else if (mainPath == "settings")
        {
            // Create resource data for settings page
            m_resourceData = std::make_unique<ResourceData>();
            m_resourceData->mimeType = "text/html";
            m_resourceData->data = R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <title>PoEOverlay Settings</title>
                    <style>
                        body {
                            font-family: Arial, sans-serif;
                            background-color: #2c3e50;
                            color: #ecf0f1;
                            margin: 0;
                            padding: 20px;
                        }
                        h1 {
                            color: #e74c3c;
                        }
                        .settings-group {
                            background-color: #34495e;
                            border-radius: 5px;
                            padding: 15px;
                            margin-bottom: 20px;
                            box-shadow: 0 4px 8px rgba(0,0,0,0.2);
                        }
                        label {
                            display: block;
                            margin-bottom: 10px;
                        }
                        input, select {
                            margin-left: 10px;
                            background-color: #2c3e50;
                            border: 1px solid #3498db;
                            color: #ecf0f1;
                            padding: 5px;
                            border-radius: 3px;
                        }
                        button {
                            background-color: #3498db;
                            color: white;
                            border: none;
                            padding: 8px 16px;
                            border-radius: 4px;
                            cursor: pointer;
                            transition: background-color 0.2s;
                        }
                        button:hover {
                            background-color: #2980b9;
                        }
                        .hotkey-box {
                            display: inline-block;
                            padding: 5px 10px;
                            background-color: #2c3e50;
                            border: 1px solid #3498db;
                            border-radius: 3px;
                            margin-left: 10px;
                            min-width: 80px;
                            text-align: center;
                        }
                        .save-button {
                            display: block;
                            margin-top: 20px;
                            padding: 10px 20px;
                            background-color: #2ecc71;
                        }
                        .save-button:hover {
                            background-color: #27ae60;
                        }
                        a {
                            color: #3498db;
                            text-decoration: none;
                        }
                        a:hover {
                            text-decoration: underline;
                        }
                        .nav-link {
                            margin-bottom: 20px;
                        }
                    </style>
                </head>
                <body>
                    <div class="nav-link">
                        <a href="poe://home">← Back to Home</a>
                    </div>
                    <h1>PoEOverlay Settings</h1>
                    <div class="settings-group">
                        <h2>General</h2>
                        <label>
                            Opacity:
                            <input type="range" min="0.1" max="1.0" step="0.1" value="0.9" id="opacity-slider">
                            <span id="opacity-value">0.9</span>
                        </label>
                        <label>
                            Start with Windows:
                            <input type="checkbox" id="start-with-windows">
                        </label>
                        <label>
                            Home Page URL:
                            <input type="text" value="poe://home" id="homepage-url" style="width: 250px;">
                        </label>
                        <label>
                            Search Engine:
                            <select id="search-engine">
                                <option value="https://www.google.com/search?q={}">Google</option>
                                <option value="https://www.bing.com/search?q={}">Bing</option>
                                <option value="https://duckduckgo.com/?q={}">DuckDuckGo</option>
                            </select>
                        </label>
                    </div>
                    <div class="settings-group">
                        <h2>Hotkeys</h2>
                        <label>
                            Toggle Overlay:
                            <span class="hotkey-box" id="toggle-hotkey">Alt+B</span>
                            <button id="change-toggle">Change</button>
                        </label>
                        <label>
                            Toggle Interaction Mode:
                            <span class="hotkey-box" id="interaction-hotkey">Alt+I</span>
                            <button id="change-interaction">Change</button>
                        </label>
                    </div>
                    <div class="settings-group">
                        <h2>Performance</h2>
                        <label>
                            Suspend browser when hidden:
                            <input type="checkbox" id="suspend-when-hidden" checked>
                        </label>
                        <label>
                            Throttle rendering when game is active:
                            <input type="checkbox" id="throttle-when-game-active" checked>
                        </label>
                    </div>
                    <div class="settings-group">
                        <h2>Privacy</h2>
                        <label>
                            Save browsing history:
                            <input type="checkbox" id="save-history" checked>
                        </label>
                        <label>
                            Save cookies between sessions:
                            <input type="checkbox" id="save-cookies" checked>
                        </label>
                        <label>
                            <button id="clear-data">Clear Browsing Data</button>
                        </label>
                    </div>
                    <button class="save-button" id="save-settings">Save Settings</button>

                    <script>
                        // Simple script to handle settings UI interactions
                        document.addEventListener('DOMContentLoaded', function() {
                            // Handle opacity slider
                            const opacitySlider = document.getElementById('opacity-slider');
                            const opacityValue = document.getElementById('opacity-value');
                            
                            opacitySlider.addEventListener('input', function() {
                                opacityValue.textContent = this.value;
                            });
                            
                            // Mock hotkey change process
                            const changeButtons = document.querySelectorAll('button[id^="change-"]');
                            changeButtons.forEach(button => {
                                button.addEventListener('click', function() {
                                    const hotkeyId = this.id.replace('change-', '') + '-hotkey';
                                    const hotkeyBox = document.getElementById(hotkeyId);
                                    
                                    hotkeyBox.textContent = 'Press a key...';
                                    
                                    // This would normally interface with the native app
                                    setTimeout(() => {
                                        hotkeyBox.textContent = hotkeyBox.textContent === 'Press a key...' 
                                            ? (hotkeyId === 'toggle-hotkey' ? 'Alt+B' : 'Alt+I') 
                                            : hotkeyBox.textContent;
                                    }, 2000);
                                });
                            });
                            
                            // Mock save function
                            document.getElementById('save-settings').addEventListener('click', function() {
                                alert('Settings saved!');
                                // In reality, this would send the settings to the native app
                            });
                            
                            // Mock clear data
                            document.getElementById('clear-data').addEventListener('click', function() {
                                if (confirm('Are you sure you want to clear all browsing data?')) {
                                    alert('Browsing data cleared!');
                                }
                            });
                        });
                    </script>
                </body>
                </html>
            )";
            m_resourceData->offset = 0;
            
            return true;
        }
        else if (mainPath == "bookmarks")
        {
            // Create resource data for bookmarks page
            m_resourceData = std::make_unique<ResourceData>();
            m_resourceData->mimeType = "text/html";
            m_resourceData->data = R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <title>PoEOverlay Bookmarks</title>
                    <style>
                        body {
                            font-family: Arial, sans-serif;
                            background-color: #2c3e50;
                            color: #ecf0f1;
                            margin: 0;
                            padding: 20px;
                        }
                        h1 {
                            color: #e74c3c;
                        }
                        .bookmark-folder {
                            background-color: #34495e;
                            border-radius: 5px;
                            padding: 15px;
                            margin-bottom: 20px;
                            box-shadow: 0 4px 8px rgba(0,0,0,0.2);
                        }
                        .bookmark-list {
                            list-style-type: none;
                            padding: 0;
                        }
                        .bookmark-item {
                            padding: 8px;
                            border-bottom: 1px solid #2c3e50;
                            display: flex;
                            justify-content: space-between;
                            align-items: center;
                        }
                        .bookmark-item:last-child {
                            border-bottom: none;
                        }
                        a {
                            color: #3498db;
                            text-decoration: none;
                        }
                        a:hover {
                            text-decoration: underline;
                        }
                        .add-button {
                            display: inline-block;
                            padding: 8px 16px;
                            background-color: #2ecc71;
                            color: white;
                            border: none;
                            border-radius: 4px;
                            cursor: pointer;
                            transition: background-color 0.2s;
                        }
                        .add-button:hover {
                            background-color: #27ae60;
                        }
                        .action-buttons {
                            display: flex;
                            gap: 5px;
                        }
                        .action-button {
                            background-color: #3498db;
                            color: white;
                            border: none;
                            padding: 4px 8px;
                            border-radius: 3px;
                            cursor: pointer;
                            font-size: 12px;
                        }
                        .action-button.delete {
                            background-color: #e74c3c;
                        }
                        .action-button:hover {
                            opacity: 0.8;
                        }
                        .nav-link {
                            margin-bottom: 20px;
                        }
                        .bookmark-header {
                            display: flex;
                            justify-content: space-between;
                            align-items: center;
                        }
                        .folder-actions {
                            display: flex;
                            gap: 5px;
                        }
                        #bookmark-form {
                            display: none;
                            background-color: #2c3e50;
                            padding: 15px;
                            border-radius: 5px;
                            margin-bottom: 20px;
                        }
                        .form-group {
                            margin-bottom: 10px;
                        }
                        .form-group label {
                            display: block;
                            margin-bottom: 5px;
                        }
                        .form-group input, .form-group select {
                            width: 100%;
                            padding: 8px;
                            background-color: #34495e;
                            border: 1px solid #3498db;
                            color: #ecf0f1;
                            border-radius: 3px;
                        }
                        .form-buttons {
                            display: flex;
                            gap: 10px;
                            margin-top: 15px;
                        }
                    </style>
                </head>
                <body>
                    <div class="nav-link">
                        <a href="poe://home">← Back to Home</a>
                    </div>
                    <h1>Bookmarks</h1>
                    <button class="add-button" id="show-add-form">Add Bookmark</button>
                    
                    <div id="bookmark-form">
                        <div class="form-group">
                            <label for="bookmark-name">Name:</label>
                            <input type="text" id="bookmark-name" placeholder="Enter bookmark name">
                        </div>
                        <div class="form-group">
                            <label for="bookmark-url">URL:</label>
                            <input type="text" id="bookmark-url" placeholder="Enter URL">
                        </div>
                        <div class="form-group">
                            <label for="bookmark-folder">Folder:</label>
                            <select id="bookmark-folder">
                                <option value="Builds">Builds</option>
                                <option value="Tools">Tools</option>
                                <option value="Official">Official</option>
                                <option value="Custom">Custom</option>
                            </select>
                        </div>
                        <div class="form-buttons">
                            <button class="add-button" id="save-bookmark">Save</button>
                            <button class="action-button" id="cancel-bookmark">Cancel</button>
                        </div>
                    </div>
                    
                    <div class="bookmark-folder" id="builds-folder">
                        <div class="bookmark-header">
                            <h2>Builds</h2>
                            <div class="folder-actions">
                                <button class="action-button" data-folder="Builds">Add to Folder</button>
                            </div>
                        </div>
                        <ul class="bookmark-list" id="builds-list">
                            <li class="bookmark-item">
                                <a href="https://www.pathofexile.com/forum/view-thread/1147951">Enki's Arc Witch</a>
                                <div class="action-buttons">
                                    <button class="action-button">Edit</button>
                                    <button class="action-button delete">Delete</button>
                                </div>
                            </li>
                            <li class="bookmark-item">
                                <a href="https://www.pathofexile.com/forum/view-thread/2486771">Bleedbow Gladiator</a>
                                <div class="action-buttons">
                                    <button class="action-button">Edit</button>
                                    <button class="action-button delete">Delete</button>
                                </div>
                            </li>
                        </ul>
                    </div>
                    
                    <div class="bookmark-folder" id="tools-folder">
                        <div class="bookmark-header">
                            <h2>Tools</h2>
                            <div class="folder-actions">
                                <button class="action-button" data-folder="Tools">Add to Folder</button>
                            </div>
                        </div>
                        <ul class="bookmark-list" id="tools-list">
                            <li class="bookmark-item">
                                <a href="https://www.pathofexile.com/trade">Official Trade Site</a>
                                <div class="action-buttons">
                                    <button class="action-button">Edit</button>
                                    <button class="action-button delete">Delete</button>
                                </div>
                            </li>
                            <li class="bookmark-item">
                                <a href="https://poe.ninja">poe.ninja</a>
                                <div class="action-buttons">
                                    <button class="action-button">Edit</button>
                                    <button class="action-button delete">Delete</button>
                                </div>
                            </li>
                            <li class="bookmark-item">
                                <a href="https://poedb.tw">PoEDB</a>
                                <div class="action-buttons">
                                    <button class="action-button">Edit</button>
                                    <button class="action-button delete">Delete</button>
                                </div>
                            </li>
                            <li class="bookmark-item">
                                <a href="https://www.craftofexile.com">Craft of Exile</a>
                                <div class="action-buttons">
                                    <button class="action-button">Edit</button>
                                    <button class="action-button delete">Delete</button>
                                </div>
                            </li>
                        </ul>
                    </div>
                    
                    <div class="bookmark-folder" id="official-folder">
                        <div class="bookmark-header">
                            <h2>Official</h2>
                            <div class="folder-actions">
                                <button class="action-button" data-folder="Official">Add to Folder</button>
                            </div>
                        </div>
                        <ul class="bookmark-list" id="official-list">
                            <li class="bookmark-item">
                                <a href="https://www.pathofexile.com">Path of Exile</a>
                                <div class="action-buttons">
                                    <button class="action-button">Edit</button>
                                    <button class="action-button delete">Delete</button>
                                </div>
                            </li>
                            <li class="bookmark-item">
                                <a href="https://www.pathofexile.com/forum">PoE Forums</a>
                                <div class="action-buttons">
                                    <button class="action-button">Edit</button>
                                    <button class="action-button delete">Delete</button>
                                </div>
                            </li>
                            <li class="bookmark-item">
                                <a href="https://www.pathofexile.com/account/view-profile">My Profile</a>
                                <div class="action-buttons">
                                    <button class="action-button">Edit</button>
                                    <button class="action-button delete">Delete</button>
                                </div>
                            </li>
                        </ul>
                    </div>

                    <script>
                        // Simple script to handle bookmark UI interactions
                        document.addEventListener('DOMContentLoaded', function() {
                            // Show/hide bookmark form
                            const showFormButton = document.getElementById('show-add-form');
                            const bookmarkForm = document.getElementById('bookmark-form');
                            const saveBookmarkButton = document.getElementById('save-bookmark');
                            const cancelBookmarkButton = document.getElementById('cancel-bookmark');
                            
                            showFormButton.addEventListener('click', function() {
                                bookmarkForm.style.display = 'block';
                                document.getElementById('bookmark-name').focus();
                            });
                            
                            cancelBookmarkButton.addEventListener('click', function() {
                                bookmarkForm.style.display = 'none';
                                // Clear form fields
                                document.getElementById('bookmark-name').value = '';
                                document.getElementById('bookmark-url').value = '';
                            });
                            
                            // Add folder buttons
                            const folderButtons = document.querySelectorAll('[data-folder]');
                            folderButtons.forEach(button => {
                                button.addEventListener('click', function() {
                                    const folder = this.getAttribute('data-folder');
                                    document.getElementById('bookmark-folder').value = folder;
                                    bookmarkForm.style.display = 'block';
                                    document.getElementById('bookmark-name').focus();
                                });
                            });
                            
                            // Mock saving bookmark
                            saveBookmarkButton.addEventListener('click', function() {
                                const name = document.getElementById('bookmark-name').value.trim();
                                const url = document.getElementById('bookmark-url').value.trim();
                                const folder = document.getElementById('bookmark-folder').value;
                                
                                if (!name || !url) {
                                    alert('Please enter both name and URL');
                                    return;
                                }
                                
                                // In a real implementation, this would add the bookmark to the app's storage
                                alert(`Bookmark added: ${name} - ${url} (${folder})`);
                                
                                // Clear and hide form
                                document.getElementById('bookmark-name').value = '';
                                document.getElementById('bookmark-url').value = '';
                                bookmarkForm.style.display = 'none';
                                
                                // In a real implementation, the page would refresh or update with the new bookmark
                            });
                            
                            // Mock delete buttons
                            const deleteButtons = document.querySelectorAll('.action-button.delete');
                            deleteButtons.forEach(button => {
                                button.addEventListener('click', function() {
                                    const bookmarkItem = this.closest('.bookmark-item');
                                    const bookmarkLink = bookmarkItem.querySelector('a');
                                    
                                    if (confirm(`Delete bookmark "${bookmarkLink.textContent}"?`)) {
                                        // In a real implementation, this would remove the bookmark from storage
                                        bookmarkItem.remove();
                                    }
                                });
                            });
                            
                            // Mock edit buttons
                            const editButtons = document.querySelectorAll('.action-button:not(.delete)');
                            editButtons.forEach(button => {
                                if (button.textContent === 'Edit') {
                                    button.addEventListener('click', function() {
                                        const bookmarkItem = this.closest('.bookmark-item');
                                        const bookmarkLink = bookmarkItem.querySelector('a');
                                        
                                        // Populate form with bookmark data
                                        document.getElementById('bookmark-name').value = bookmarkLink.textContent;
                                        document.getElementById('bookmark-url').value = bookmarkLink.href;
                                        
                                        // Determine folder from parent element
                                        const folderId = bookmarkItem.closest('.bookmark-folder').id;
                                        let folder = 'Custom';
                                        if (folderId === 'builds-folder') folder = 'Builds';
                                        if (folderId === 'tools-folder') folder = 'Tools';
                                        if (folderId === 'official-folder') folder = 'Official';
                                        
                                        document.getElementById('bookmark-folder').value = folder;
                                        
                                        // Show form
                                        bookmarkForm.style.display = 'block';
                                        document.getElementById('bookmark-name').focus();
                                        
                                        // In a real implementation, we'd mark this as an edit operation
                                        // rather than an add operation
                                    });
                                }
                            });
                        });
                    </script>
                </body>
                </html>
            )";
            m_resourceData->offset = 0;
            
            return true;
        }
        else if (mainPath == "error")
        {
            // Create resource data for error page
            m_resourceData = std::make_unique<ResourceData>();
            m_resourceData->mimeType = "text/html";
            
            // Parse error details from query string
            std::string errorCode = "Unknown";
            std::string errorMessage = "An unknown error occurred";
            std::string errorUrl = "";
            
            if (!queryParams.empty()) {
                // Very basic query param parsing - in a real implementation, use a proper parser
                size_t codePos = queryParams.find("code=");
                if (codePos != std::string::npos) {
                    size_t codeEnd = queryParams.find('&', codePos);
                    errorCode = queryParams.substr(codePos + 5, codeEnd == std::string::npos ? 
                        std::string::npos : codeEnd - codePos - 5);
                }
                
                size_t msgPos = queryParams.find("message=");
                if (msgPos != std::string::npos) {
                    size_t msgEnd = queryParams.find('&', msgPos);
                    errorMessage = queryParams.substr(msgPos + 8, msgEnd == std::string::npos ? 
                        std::string::npos : msgEnd - msgPos - 8);
                    
                    // Simple URL decoding for spaces
                    size_t pos = 0;
                    while ((pos = errorMessage.find("+", pos)) != std::string::npos) {
                        errorMessage.replace(pos, 1, " ");
                        pos++;
                    }
                }
                
                size_t urlPos = queryParams.find("url=");
                if (urlPos != std::string::npos) {
                    size_t urlEnd = queryParams.find('&', urlPos);
                    errorUrl = queryParams.substr(urlPos + 4, urlEnd == std::string::npos ? 
                        std::string::npos : urlEnd - urlPos - 4);
                    
                    // Simple URL decoding for spaces
                    size_t pos = 0;
                    while ((pos = errorUrl.find("+", pos)) != std::string::npos) {
                        errorUrl.replace(pos, 1, " ");
                        pos++;
                    }
                }
            }
            
            // Generate the error page
            m_resourceData->data = R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <title>PoEOverlay - Error</title>
                    <style>
                        body {
                            font-family: Arial, sans-serif;
                            background-color: #2c3e50;
                            color: #ecf0f1;
                            margin: 0;
                            padding: 20px;
                            text-align: center;
                        }
                        .error-container {
                            background-color: #34495e;
                            border-radius: 5px;
                            padding: 20px;
                            margin: 0 auto;
                            max-width: 600px;
                            box-shadow: 0 4px 8px rgba(0,0,0,0.2);
                        }
                        h1 {
                            color: #e74c3c;
                        }
                        .error-code {
                            font-family: monospace;
                            background-color: #2c3e50;
                            padding: 5px 10px;
                            border-radius: 3px;
                            display: inline-block;
                            margin-bottom: 20px;
                        }
                        .error-url {
                            word-break: break-all;
                            background-color: #2c3e50;
                            padding: 10px;
                            border-radius: 3px;
                            margin: 10px 0;
                            text-align: left;
                        }
                        a {
                            color: #3498db;
                            text-decoration: none;
                        }
                        a:hover {
                            text-decoration: underline;
                        }
                        .buttons {
                            margin-top: 20px;
                        }
                        .button {
                            display: inline-block;
                            padding: 8px 16px;
                            margin: 0 5px;
                            background-color: #3498db;
                            color: white;
                            border-radius: 4px;
                            text-decoration: none;
                        }
                        .button:hover {
                            background-color: #2980b9;
                            text-decoration: none;
                        }
                    </style>
                </head>
                <body>
                    <div class="error-container">
                        <h1>Page Load Error</h1>
                        <div class="error-code">Error )" + errorCode + R"(</div>
                        <p>)" + errorMessage + R"(</p>
                        )";
            
            if (!errorUrl.empty()) {
                m_resourceData->data += R"(
                        <p>Failed to load:</p>
                        <div class="error-url">)" + errorUrl + R"(</div>
                        )";
            }
            
            m_resourceData->data += R"(
                        <div class="buttons">
                            <a href=")" + (errorUrl.empty() ? "poe://home" : errorUrl) + R"(" class="button">Try Again</a>
                            <a href="poe://home" class="button">Go Home</a>
                        </div>
                    </div>
                </body>
                </html>
            )";
            
            m_resourceData->offset = 0;
            return true;
        }
    }
    
    // Not handled
    return false;
}

std::string ResourceHandler::GetMimeType(const std::string& extension)
{
    // Common MIME types
    if (extension == "html" || extension == "htm")
        return "text/html";
    else if (extension == "css")
        return "text/css";
    else if (extension == "js")
        return "application/javascript";
    else if (extension == "json")
        return "application/json";
    else if (extension == "png")
        return "image/png";
    else if (extension == "jpg" || extension == "jpeg")
        return "image/jpeg";
    else if (extension == "gif")
        return "image/gif";
    else if (extension == "svg")
        return "image/svg+xml";
    else if (extension == "ico")
        return "image/x-icon";
    else if (extension == "txt")
        return "text/plain";
    else if (extension == "xml")
        return "application/xml";
    else
        return "application/octet-stream";
}

template<typename... Args>
void ResourceHandler::Log(int level, const std::string& fmt, const Args&... args)
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
        std::cerr << "ResourceHandler log error: " << e.what() << std::endl;
    }
}

} // namespace poe