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
    
    // Example handling for "poe" scheme
    if (scheme == "poe")
    {
        // Handle special poe:// URLs
        if (path == "home")
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
                    </style>
                </head>
                <body>
                    <h1>PoEOverlay</h1>
                    <div class="card">
                        <h2>Quick Links</h2>
                        <ul>
                            <li><a href="https://www.pathofexile.com">Official Path of Exile Website</a></li>
                            <li><a href="https://www.pathofexile.com/trade">Official Trade Site</a></li>
                            <li><a href="https://poe.ninja">poe.ninja</a></li>
                            <li><a href="https://poedb.tw">PoEDB</a></li>
                        </ul>
                    </div>
                    <div class="card">
                        <h2>Recent Builds</h2>
                        <p>Your recent build bookmarks will appear here.</p>
                    </div>
                </body>
                </html>
            )";
            m_resourceData->offset = 0;
            
            return true;
        }
        else if (path == "settings")
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
                        }
                        button {
                            background-color: #3498db;
                            color: white;
                            border: none;
                            padding: 8px 16px;
                            border-radius: 4px;
                            cursor: pointer;
                        }
                        button:hover {
                            background-color: #2980b9;
                        }
                    </style>
                </head>
                <body>
                    <h1>PoEOverlay Settings</h1>
                    <div class="settings-group">
                        <h2>General</h2>
                        <label>
                            Opacity:
                            <input type="range" min="0.1" max="1.0" step="0.1" value="0.9">
                        </label>
                        <label>
                            Start with Windows:
                            <input type="checkbox">
                        </label>
                    </div>
                    <div class="settings-group">
                        <h2>Hotkeys</h2>
                        <label>
                            Toggle Overlay:
                            <input type="text" value="Alt+B" readonly>
                            <button>Change</button>
                        </label>
                        <label>
                            Toggle Interaction Mode:
                            <input type="text" value="Alt+I" readonly>
                            <button>Change</button>
                        </label>
                    </div>
                    <button>Save Settings</button>
                </body>
                </html>
            )";
            m_resourceData->offset = 0;
            
            return true;
        }
        else if (path == "bookmarks")
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
                        }
                        a {
                            color: #3498db;
                            text-decoration: none;
                        }
                        a:hover {
                            text-decoration: underline;
                        }
                        .add-button {
                            background-color: #2ecc71;
                            color: white;
                            border: none;
                            padding: 8px 16px;
                            border-radius: 4px;
                            cursor: pointer;
                        }
                        .add-button:hover {
                            background-color: #27ae60;
                        }
                    </style>
                </head>
                <body>
                    <h1>Bookmarks</h1>
                    <button class="add-button">Add Bookmark</button>
                    <div class="bookmark-folder">
                        <h2>Builds</h2>
                        <ul class="bookmark-list">
                            <li class="bookmark-item"><a href="https://www.pathofexile.com/forum/view-thread/1147951">Enki's Arc Witch</a></li>
                            <li class="bookmark-item"><a href="https://www.pathofexile.com/forum/view-thread/2486771">Bleedbow Gladiator</a></li>
                        </ul>
                    </div>
                    <div class="bookmark-folder">
                        <h2>Tools</h2>
                        <ul class="bookmark-list">
                            <li class="bookmark-item"><a href="https://www.pathofexile.com/trade">Official Trade Site</a></li>
                            <li class="bookmark-item"><a href="https://poe.ninja">poe.ninja</a></li>
                            <li class="bookmark-item"><a href="https://poedb.tw">PoEDB</a></li>
                        </ul>
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