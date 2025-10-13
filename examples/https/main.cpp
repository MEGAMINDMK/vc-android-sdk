#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <android/log.h>
#include "https.h"

// Clean logging macros
#define LOG_TAG "https"
#define logi(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define logw(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
// Registering mod with AML using macro provided by AML framework
MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

// Global variable: handle for the GTA VC shared library
void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

void TestHTTPSFeatures() {
    logi("=== Testing HTTPS Features ===");
    
    std::string response;
    
    // Test GET request
    if (HTTPSClient::GET("httpbin.org", "/get?megamind=awesome", response)) {
        logi("✓ GET Request Successful!");
        logi("Response: %s", response.c_str());
    }
    
    // Test POST request
    std::string postData = "{\"mod_name\":\"ViceCityMod\",\"version\":\"1.0\"}";
    if (HTTPSClient::POST("httpbin.org", "/post", postData, response)) {
        logi("✓ POST Request Successful!");
        logi("Response: %s", response.c_str());
    }
    
    // Test downloading a file
    if (HTTPSClient::GET("raw.githubusercontent.com", "/MEGAMINDMK/vc-android-sdk/refs/heads/main/README.md", response)) {
        logi("✓ File Download Successful!");
        logi("Content: %s", response.c_str());
    }
}

// Hook declarations
DECL_HOOKv(SetBigMessage, unsigned short* text , unsigned short style)
{
    char16_t myString[] = u"MEGAMIND HTTPS";
    SetBigMessage(reinterpret_cast<unsigned short*>(myString), 1);
}

DECL_HOOKv(SetHelpMessage, unsigned short* text, bool flag1, bool flag2, bool flag3)
{
    char16_t myString2[] = u"HTTPS MOD v1.0";
    SetHelpMessage(reinterpret_cast<unsigned short*>(myString2), true, false, false);
}

// The main entry point called by AML when the mod is loaded
extern "C" void OnModLoad()
{
    logi("MEGAMIND Vice City Mod loading...");
    
    // Initialize HTTPS client
    if (!HTTPSClient::Initialize()) {
        loge("Failed to initialize HTTPS client");
        return;
    }
    
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (pGTAVC && hGTAVC)
    {
        HOOKSYM(SetBigMessage, hGTAVC, "_ZN4CHud13SetBigMessageEPtt");
        HOOKSYM(SetHelpMessage, hGTAVC, "_ZN4CHud14SetHelpMessageEPtbbb");
        logi("✓ Hooks installed successfully!");
    }
    else
    {
        loge("✗ Failed to load libGTAVC.so!");
        return;
    }
    
    // Test HTTPS functionality
    TestHTTPSFeatures();
    
    logi("🎉 MEGAMIND HTTPS Mod initialized!");
}

// Cleanup when mod unloads
extern "C" void OnModUnload()
{
    HTTPSClient::Cleanup();
    logi("MEGAMIND Vice City Mod unloaded");
}