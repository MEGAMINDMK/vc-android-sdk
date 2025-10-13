#include "https.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <android/log.h>
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

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
        
        // Disable old SSL versions, prefer TLS
        SSL_CTX_set_options(sslContext, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
        
        // Skip certificate verification for testing
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

    static bool PerformHTTPSRequest(const std::string& host, const std::string& request, std::string& response) {
        if (!IsAvailable()) {
            loge("HTTPS client not initialized");
            return false;
        }

        // Resolve hostname to IP
        struct hostent* he = gethostbyname(host.c_str());
        if (he == NULL) {
            loge("DNS resolution failed for: %s", host.c_str());
            return false;
        }
        
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, he->h_addr_list[0], ip_str, sizeof(ip_str));
        logi("Resolved %s -> %s", host.c_str(), ip_str);

        // Create socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            loge("Socket creation failed");
            return false;
        }

        // Connect to IP
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(443);
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 15;
        timeout.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        logi("Connecting to %s:443", host.c_str());
        
        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            loge("TCP connection failed: %s", strerror(errno));
            close(sockfd);
            return false;
        }

        logi("TCP connection successful");

        // Create SSL object
        SSL* ssl = SSL_new(sslContext);
        if (!ssl) {
            loge("SSL_new failed");
            close(sockfd);
            return false;
        }

        SSL_set_fd(ssl, sockfd);
        SSL_set_tlsext_host_name(ssl, host.c_str());

        logi("Performing SSL handshake...");
        if (SSL_connect(ssl) != 1) {
            loge("SSL handshake failed: %s", ERR_error_string(ERR_get_error(), NULL));
            SSL_free(ssl);
            close(sockfd);
            return false;
        }

        logi("SSL handshake successful! Cipher: %s", SSL_get_cipher(ssl));

        // Send request
        int bytes_sent = SSL_write(ssl, request.c_str(), request.length());
        if (bytes_sent <= 0) {
            loge("SSL_write failed: %s", ERR_error_string(ERR_get_error(), NULL));
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(sockfd);
            return false;
        }

        logi("Sent %d bytes, receiving response...", bytes_sent);

        // Receive response
        char buffer[4096];
        int bytes_read;
        response.clear();

        while ((bytes_read = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            response.append(buffer, bytes_read);
        }

        // Cleanup
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sockfd);

        if (response.empty()) {
            loge("No response received");
            return false;
        }

        // Extract body from response
        size_t header_end = response.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            response = response.substr(header_end + 4);
        }

        logi("HTTPS response received: %d bytes", response.length());
        return true;
    }

    bool GET(const std::string& host, const std::string& path, std::string& response) {
        std::string request = "GET " + path + " HTTP/1.1\r\n"
                             "Host: " + host + "\r\n"
                             "User-Agent: MEGAMIND-MOD/1.0\r\n"
                             "Accept: */*\r\n"
                             "Connection: close\r\n"
                             "\r\n";

        return PerformHTTPSRequest(host, request, response);
    }

    bool POST(const std::string& host, const std::string& path, const std::string& data, std::string& response) {
        return POST(host, path, data, "application/json", response);
    }

    bool POST(const std::string& host, const std::string& path, const std::string& data, 
              const std::string& contentType, std::string& response) {
        std::string request = "POST " + path + " HTTP/1.1\r\n"
                             "Host: " + host + "\r\n"
                             "User-Agent: MEGAMIND-MOD/1.0\r\n"
                             "Content-Type: " + contentType + "\r\n"
                             "Content-Length: " + std::to_string(data.length()) + "\r\n"
                             "Connection: close\r\n"
                             "\r\n"
                             + data;

        return PerformHTTPSRequest(host, request, response);
    }

} // namespace HTTPSClient