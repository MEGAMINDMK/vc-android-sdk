#ifndef HTTPS_CLIENT_H
#define HTTPS_CLIENT_H

#include <string>

// Clean logging macros
#define LOG_TAG "https"
#define logi(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace HTTPSClient {
    
    // Initialize HTTPS client (call once at startup)
    bool Initialize();
    
    // Perform HTTPS GET request
    bool GET(const std::string& host, const std::string& path, std::string& response);
    
    // Perform HTTPS POST request
    bool POST(const std::string& host, const std::string& path, const std::string& data, std::string& response);
    
    // Perform HTTPS POST request with custom content type
    bool POST(const std::string& host, const std::string& path, const std::string& data, 
              const std::string& contentType, std::string& response);
    
    // Cleanup resources (call when mod unloads)
    void Cleanup();
    
    // Utility function to check if HTTPS is available
    bool IsAvailable();

} // namespace HTTPSClient

#endif // HTTPS_CLIENT_H