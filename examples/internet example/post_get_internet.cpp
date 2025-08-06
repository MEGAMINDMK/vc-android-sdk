#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <android/log.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>
#include <stdint.h>
#include <dlfcn.h>
#include <string>
#include <vector>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <thread>
#include <mutex>
#include <chrono>
#ifndef LOG_TAG
#define LOG_TAG "ViceCityMod"
#endif
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

void*     hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

static std::string fetchedMessage;
static std::vector<char16_t> g_message;  // Only this, not u16string!
static std::chrono::steady_clock::time_point lastFetchTime;
static const std::chrono::seconds fetchInterval(5);
static bool shown = false;

static std::mutex messageMutex;
static std::atomic<bool> isFetching = false;


//------------------------------------------------------------------------------
// Trim whitespace/newlines from both ends of a string
static std::string trim(const std::string& str)
{
    const char* ws = " \t\n\r";
    size_t start = str.find_first_not_of(ws);
    size_t end = str.find_last_not_of(ws);

    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

//------------------------------------------------------------------------------
// Simple HTTP GET to fetch message
static std::string GetHttpBody(const char* host, int port, const char* path)
{
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char portStr[6];
    snprintf(portStr, sizeof(portStr), "%d", port);

    if (getaddrinfo(host, portStr, &hints, &res) != 0)
    {
        LOGE("DNS lookup failed for %s", host);
        return {};
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0)
    {
        LOGE("socket() failed");
        freeaddrinfo(res);
        return {};
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0)
    {
        LOGE("connect() to %s:%d failed", host, port);
        close(sock);
        freeaddrinfo(res);
        return {};
    }
    freeaddrinfo(res);

    char req[512];
    int len = snprintf(req, sizeof(req),
                       "GET %s HTTP/1.1\r\n"
                       "Host: %s\r\n"
                       "Connection: close\r\n\r\n", path, host);
    send(sock, req, len, 0);

    std::vector<char> buf(4096);
    int rec = recv(sock, buf.data(), (int)buf.size() - 1, 0);
    close(sock);

    if (rec <= 0)
    {
        LOGE("recv() failed or no data");
        return {};
    }

    buf[rec] = '\0';
    std::string resp(buf.data());

    auto hdr_end = resp.find("\r\n\r\n");
    if (hdr_end != std::string::npos)
        return resp.substr(hdr_end + 4);

    return resp;
}


static std::string postHttpBody(const char* host, int port, const char* path, const std::string& postData)
{
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char portStr[6];
    snprintf(portStr, sizeof(portStr), "%d", port);

    if (getaddrinfo(host, portStr, &hints, &res) != 0)
    {
        LOGE("DNS lookup failed for %s", host);
        return {};
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0)
    {
        LOGE("socket() failed");
        freeaddrinfo(res);
        return {};
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0)
    {
        LOGE("connect() to %s:%d failed", host, port);
        close(sock);
        freeaddrinfo(res);
        return {};
    }
    freeaddrinfo(res);

    char header[1024];
    int len = snprintf(header, sizeof(header),
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n", path, host, postData.size());

    std::vector<char> requestData(header, header + len);
    requestData.insert(requestData.end(), postData.begin(), postData.end());

    send(sock, requestData.data(), (int)requestData.size(), 0);

    std::vector<char> buf(4096);
    int rec = recv(sock, buf.data(), (int)buf.size() - 1, 0);
    close(sock);

    if (rec <= 0)
    {
        LOGE("recv() failed or no data");
        return {};
    }

    buf[rec] = '\0';
    std::string resp(buf.data());

    auto hdr_end = resp.find("\r\n\r\n");
    if (hdr_end != std::string::npos)
        return resp.substr(hdr_end + 4);

    return resp;
}

//------------------------------------------------------------------------------
// Convert basic UTF-8 (ASCII) to UTF-16 (char16_t)
static std::u16string utf8ToUtf16Basic(const std::string& utf8)
{
    std::u16string out;
    for (unsigned char c : utf8)
    {
        if (c >= 32 && c < 127) // printable ASCII
            out.push_back((char16_t)c);
    }
    return out;
}

//------------------------------------------------------------------------------



void fetchMessageAsync() {
    std::thread([](){
        std::string body = GetHttpBody("yourdomain.com", 80, "/file.php"); // to get data
        std::string trimmed = trim(body);
        auto msg = utf8ToUtf16Basic(trimmed);
        msg.push_back(u'\0');

        {
            std::lock_guard<std::mutex> lock(messageMutex);
            if (trimmed != fetchedMessage) {
                fetchedMessage = trimmed;
                g_message = std::vector<char16_t>(msg.begin(), msg.end());
                shown = false;
                LOGD("Fetched new message: %s", trimmed.c_str());
            } else {
                LOGD("Fetched message unchanged.");
            }
            lastFetchTime = std::chrono::steady_clock::now();
        }

        isFetching.store(false);
    }).detach(); // Thread is detached — runs independently
}

DECL_HOOKv(SetBigMessage, unsigned short* text, unsigned short style)
{
    auto now = std::chrono::steady_clock::now();

    if (!isFetching.load() && std::chrono::duration_cast<std::chrono::seconds>(now - lastFetchTime) >= fetchInterval) {
        bool expected = false;
        if (isFetching.compare_exchange_strong(expected, true)) {
            fetchMessageAsync();
        }
    }

    std::lock_guard<std::mutex> lock(messageMutex);

    if (g_message.empty()) {
        LOGD("g_message is empty, using fallback message.");
        char16_t fallback[] = u"MEGAMIND";
        SetBigMessage(reinterpret_cast<unsigned short*>(fallback), 2);
    }
    else if (!shown) {
        LOGD("Displaying fetched message for SetBigMessage: %s", fetchedMessage.c_str());
        SetBigMessage(reinterpret_cast<unsigned short*>(g_message.data()), 2);
		// send data
		std::string response = postHttpBody("yourdomain.com", 80, "/submit.php", "name=MEGAMIND&score=1997"); // to post data to internet
        LOGD("POST Response: %s", response.c_str());
        shown = true;
    }
	
	

}


//------------------------------------------------------------------------------
// Entry point
extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (pGTAVC && hGTAVC)
    {
        HOOKSYM(SetBigMessage, hGTAVC, "_ZN4CHud13SetBigMessageEPtt");
        LOGD("SetBigMessage hooked successfully");
    }
    else
    {
        LOGE("Failed to get libGTAVC");
    }
}
