#ifndef HTTPS_CLIENT_H
#define HTTPS_CLIENT_H

#include <string>
#include <vector>
#include <android/log.h>

// Clean logging macros
#define LOG_TAG "internetimg"
#define logi(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace HTTPSClient {

    struct Header {
        std::string name;
        std::string value;
        Header(const std::string& n, const std::string& v) : name(n), value(v) {}
    };

    bool Initialize();
    void Cleanup();
    bool IsAvailable();

    bool GET(const std::string& host, const std::string& path, std::string& response);
    bool GET(const std::string& host, const std::string& path, std::string& response, 
             const std::vector<Header>& headers);

    bool POST(const std::string& host, const std::string& path, const std::string& data, std::string& response);
    bool POST(const std::string& host, const std::string& path, const std::string& data,
              const std::string& contentType, std::string& response);
    bool POST(const std::string& host, const std::string& path, const std::string& data,
              const std::string& contentType, const std::string& authorization, std::string& response);
    bool POST(const std::string& host, const std::string& path, const std::string& data,
              const std::vector<Header>& headers, std::string& response);

    bool DownloadFile(const std::string& url, std::vector<uint8_t>& output);
    bool DownloadToFile(const std::string& url, const std::string& filepath);

} // namespace HTTPSClient

#endif // HTTPS_CLIENT_H
