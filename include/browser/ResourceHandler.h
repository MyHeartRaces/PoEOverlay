#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <include/cef_resource_handler.h>
#include "core/Application.h"

namespace poe {

// Forward declarations
class Application;
class CefManager;

/**
 * @class ResourceHandler
 * @brief Custom handler for CEF resource requests.
 * 
 * This class allows the application to intercept and handle resource
 * requests made by the browser, such as custom protocols or local resources.
 */
class ResourceHandler : public CefResourceHandler {
public:
    /**
     * @brief Constructor for the ResourceHandler class.
     * @param app Reference to the main application instance.
     * @param cefManager Reference to the CEF manager.
     */
    ResourceHandler(Application& app, CefManager& cefManager);

    // CefResourceHandler methods
    bool Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback) override;
    bool ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) override;
    void GetResponseHeaders(CefRefPtr<CefResponse> response, int64& response_length, CefString& redirectUrl) override;
    bool Skip(int64 bytes_to_skip, int64& bytes_skipped, CefRefPtr<CefResourceSkipCallback> callback) override;
    bool Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> callback) override;
    bool ReadResponse(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefCallback> callback) override;
    void Cancel() override;

    /**
     * @brief Registers a custom scheme handler.
     * @param scheme The scheme to register (e.g., "poe").
     * @return True if registration was successful, false otherwise.
     */
    bool RegisterCustomScheme(const std::string& scheme);

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

    /**
     * @struct ResourceData
     * @brief Stores data for a resource request.
     */
    struct ResourceData {
        std::string mimeType;       ///< MIME type of the resource
        std::string data;           ///< Resource data
        size_t offset = 0;          ///< Current offset in the data
    };

    /**
     * @brief Handles a request for a custom scheme.
     * @param request The request to handle.
     * @param scheme The scheme being requested.
     * @param path The path being requested.
     * @return True if the request was handled, false otherwise.
     */
    bool HandleCustomScheme(CefRefPtr<CefRequest> request, const std::string& scheme, const std::string& path);

    /**
     * @brief Gets the MIME type for a file extension.
     * @param extension The file extension.
     * @return The MIME type, or "application/octet-stream" if unknown.
     */
    std::string GetMimeType(const std::string& extension);

    // Required for IMPLEMENT_REFCOUNTING
    IMPLEMENT_REFCOUNTING(ResourceHandler);

    Application& m_app;                       ///< Reference to the main application
    CefManager& m_cefManager;                 ///< Reference to the CEF manager
    
    std::unordered_map<std::string, bool> m_registeredSchemes; ///< Map of registered schemes
    
    // Current request state
    std::unique_ptr<ResourceData> m_resourceData; ///< Data for the current resource request
    bool m_requestComplete = false;           ///< Whether the request is complete
};

} // namespace poe