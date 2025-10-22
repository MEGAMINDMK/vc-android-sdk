#include "https.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <fstream>
#include <sstream>

namespace HTTPSClient {

    static SSL_CTX* sslContext = nullptr;
    static bool sslInitialized = false;

    bool Initialize() {
        if (sslInitialized) return true;

        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();

        sslContext = SSL_CTX_new(SSLv23_client_method());
        if (!sslContext) {
            loge("Failed to create SSL context");
            return false;
        }

        SSL_CTX_set_options(sslContext, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
        SSL_CTX_set_verify(sslContext, SSL_VERIFY_NONE, NULL);

        sslInitialized = true;
        logi("HTTPS client initialized successfully");
        return true;
    }

    void Cleanup() {
        if (sslContext) {
            SSL_CTX_free(sslContext);
            sslContext = nullptr;
        }
        sslInitialized = false;
        logi("HTTPS client cleaned up");
    }

    bool IsAvailable() {
        return sslInitialized && sslContext;
    }

    static bool ParseURL(const std::string& url, std::string& host, std::string& path, int& port) {
        port = 443;
        size_t start = 0;

        if (url.find("https://") == 0) start = 8;
        else if (url.find("http://") == 0) { start = 7; port = 80; }
        else return false;

        size_t slash_pos = url.find('/', start);
        if (slash_pos == std::string::npos) {
            host = url.substr(start);
            path = "/";
        } else {
            host = url.substr(start, slash_pos - start);
            path = url.substr(slash_pos);
        }

        size_t colon_pos = host.find(':');
        if (colon_pos != std::string::npos) {
            port = std::stoi(host.substr(colon_pos + 1));
            host = host.substr(0, colon_pos);
        }

        return true;
    }

    static std::string BuildRequest(const std::string& method, const std::string& host,
                                    const std::string& path, const std::vector<Header>& headers,
                                    const std::string& body = "") {
        std::ostringstream req;
        req << method << " " << path << " HTTP/1.1\r\n";
        req << "Host: " << host << "\r\n";
        req << "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
               "AppleWebKit/537.36 (KHTML, like Gecko) "
               "Chrome/127.0.0.1 Safari/537.36\r\n";
        req << "Accept: */*\r\n";
        req << "Accept-Encoding: identity\r\n";
        req << "Connection: close\r\n";

        for (const auto& h : headers)
            req << h.name << ": " << h.value << "\r\n";

        if (!body.empty())
            req << "Content-Length: " << body.length() << "\r\n";

        req << "\r\n";
        if (!body.empty()) req << body;
        return req.str();
    }

    static bool PerformHTTPSRequest(const std::string& host, int port, const std::string& request,
                                    std::string& response, bool isBinary = false, int depth = 0) {
        if (depth > 5) { loge("Too many redirects"); return false; }

        if (!IsAvailable()) {
            loge("HTTPS client not initialized");
            return false;
        }

        struct hostent* he = gethostbyname(host.c_str());
        if (!he) { loge("DNS failed for %s", host.c_str()); return false; }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, he->h_addr_list[0], ip_str, sizeof(ip_str));
        logi("Resolved %s -> %s", host.c_str(), ip_str);

        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) { loge("Socket creation failed"); return false; }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

        struct timeval timeout{30, 0};
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        logi("Connecting to %s:%d", host.c_str(), port);
        if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            loge("TCP connect failed: %s", strerror(errno));
            close(sockfd);
            return false;
        }

        SSL* ssl = nullptr;
        if (port == 443) {
            ssl = SSL_new(sslContext);
            SSL_set_fd(ssl, sockfd);
            SSL_set_tlsext_host_name(ssl, host.c_str());
            logi("Performing SSL handshake...");
            if (SSL_connect(ssl) != 1) {
                loge("SSL handshake failed: %s", ERR_error_string(ERR_get_error(), nullptr));
                SSL_free(ssl);
                close(sockfd);
                return false;
            }
            logi("SSL handshake successful! Cipher: %s", SSL_get_cipher(ssl));
        }

        int bytes_sent = ssl ? SSL_write(ssl, request.c_str(), request.size())
                             : send(sockfd, request.c_str(), request.size(), 0);
        if (bytes_sent <= 0) {
            loge("Send failed");
            if (ssl) { SSL_shutdown(ssl); SSL_free(ssl); }
            close(sockfd);
            return false;
        }

        logi("Sent %d bytes, receiving response...", bytes_sent);

        char buffer[8192];
        std::string fullResponse;
        int bytes_read;
        while ((bytes_read = ssl ? SSL_read(ssl, buffer, sizeof(buffer))
                                 : recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
            fullResponse.append(buffer, bytes_read);
        }

        if (ssl) { SSL_shutdown(ssl); SSL_free(ssl); }
        close(sockfd);

        if (fullResponse.empty()) {
            loge("No response received");
            return false;
        }

        size_t header_end = fullResponse.find("\r\n\r\n");
        std::string headers = (header_end != std::string::npos)
                                ? fullResponse.substr(0, header_end)
                                : fullResponse;
        logi("Response headers:\n%s", headers.c_str());

        // Redirect detection
        if (headers.find("HTTP/1.1 3") != std::string::npos) {
            size_t loc = headers.find("Location: ");
            if (loc != std::string::npos) {
                size_t end = headers.find("\r\n", loc);
                std::string redirect = headers.substr(loc + 10, end - (loc + 10));
                logi("Redirecting to: %s", redirect.c_str());

                std::string rHost, rPath;
                int rPort;
                if (ParseURL(redirect, rHost, rPath, rPort)) {
                    std::vector<Header> hdrs;
                    std::string req = BuildRequest("GET", rHost, rPath, hdrs);
                    return PerformHTTPSRequest(rHost, rPort, req, response, isBinary, depth + 1);
                }
            }
        }

        response = (header_end != std::string::npos)
                     ? fullResponse.substr(header_end + 4)
                     : fullResponse;
        logi("Response received: %d bytes", (int)response.length());
        return true;
    }

    bool GET(const std::string& host, const std::string& path, std::string& response) {
        std::vector<Header> headers;
        std::string req = BuildRequest("GET", host, path, headers);
        return PerformHTTPSRequest(host, 443, req, response);
    }

    bool GET(const std::string& host, const std::string& path, std::string& response,
             const std::vector<Header>& headers) {
        std::string req = BuildRequest("GET", host, path, headers);
        return PerformHTTPSRequest(host, 443, req, response);
    }

    bool POST(const std::string& host, const std::string& path, const std::string& data, std::string& response) {
        std::vector<Header> h = {{"Content-Type", "application/json"}};
        std::string req = BuildRequest("POST", host, path, h, data);
        return PerformHTTPSRequest(host, 443, req, response);
    }

    bool POST(const std::string& host, const std::string& path, const std::string& data,
              const std::string& contentType, std::string& response) {
        std::vector<Header> h = {{"Content-Type", contentType}};
        std::string req = BuildRequest("POST", host, path, h, data);
        return PerformHTTPSRequest(host, 443, req, response);
    }

    bool POST(const std::string& host, const std::string& path, const std::string& data,
              const std::string& contentType, const std::string& authorization, std::string& response) {
        std::vector<Header> h = {
            {"Content-Type", contentType},
            {"Authorization", "Bearer " + authorization}
        };
        std::string req = BuildRequest("POST", host, path, h, data);
        return PerformHTTPSRequest(host, 443, req, response);
    }

    bool POST(const std::string& host, const std::string& path, const std::string& data,
              const std::vector<Header>& headers, std::string& response) {
        std::string req = BuildRequest("POST", host, path, headers, data);
        return PerformHTTPSRequest(host, 443, req, response);
    }

    bool DownloadFile(const std::string& url, std::vector<uint8_t>& output) {
        std::string host, path;
        int port;
        if (!ParseURL(url, host, path, port)) {
            loge("Invalid URL: %s", url.c_str());
            return false;
        }

        std::vector<Header> headers;
        std::string req = BuildRequest("GET", host, path, headers);
        std::string resp;
        if (!PerformHTTPSRequest(host, port, req, resp, true))
            return false;

        output.assign(resp.begin(), resp.end());
        logi("Downloaded %zu bytes from %s", output.size(), url.c_str());
        return true;
    }

    bool DownloadToFile(const std::string& url, const std::string& filepath) {
        std::vector<uint8_t> data;
        if (!DownloadFile(url, data)) return false;

        std::ofstream file(filepath, std::ios::binary);
        if (!file) {
            loge("Failed to open file: %s", filepath.c_str());
            return false;
        }
        file.write((const char*)data.data(), data.size());
        file.close();
        logi("Saved %zu bytes to %s", data.size(), filepath.c_str());
        return true;
    }

} // namespace HTTPSClient
