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
#include <string>
#include <vector>

#include "rwimage.h"

// Clean logging macros
#define LOG_TAG "internetimg"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Registering mod with AML using macro provided by AML framework
MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

// Pointers to GTA functions
void* pDraw = nullptr;
void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

// Screen resolution function pointers
typedef int (*OS_ScreenGetWidthFn)(void);
typedef int (*OS_ScreenGetHeightFn)(void);
OS_ScreenGetWidthFn OS_ScreenGetWidth = nullptr;
OS_ScreenGetHeightFn OS_ScreenGetHeight = nullptr;

// Structures
struct RwV3d
{
    float x, y, z;
};

struct CVector
{
    float x, y, z;
};

struct CPed
{
    void* vtable;
    char pad[0x30];
    CVector vecPosition;
};

// World position function pointers
typedef bool (*CSprite_CalcScreenCoorsFn)(const RwV3d&, RwV3d*, float*, float*, bool);
typedef CPed* (*FindPlayerPedFn)();

// Function pointers
CSprite_CalcScreenCoorsFn CSprite_CalcScreenCoors = nullptr;
FindPlayerPedFn FindPlayerPed = nullptr;

// Multiple global image objects
RWImagechatpng gImageHead;      // Image on player's head (world position)
RWImagechatpng gImageWorld;     // Image at fixed world position
RWImagechatpng gImageScreen;    // Image on screen (HUD position)

// --- Get Screen Dimensions ---
int GetScreenWidth() {
    return OS_ScreenGetWidth ? OS_ScreenGetWidth() : 640;
}

int GetScreenHeight() {
    return OS_ScreenGetHeight ? OS_ScreenGetHeight() : 448;
}

// --- World to Screen Conversion ---
bool WorldToScreen(const CVector& worldPos, float& screenX, float& screenY)
{
    if (!CSprite_CalcScreenCoors) return false;
    RwV3d in = { worldPos.x, worldPos.y, worldPos.z };
    RwV3d out;
    float w, h;
    if (CSprite_CalcScreenCoors(in, &out, &w, &h, true))
    {
        screenX = out.x;
        screenY = out.y;
        return true;
    }
    return false;
}

// --- Download + Load Image Function ---
bool LoadImageFromHTTPS(RWImagechatpng& image, const char* host, const char* path, float x, float y, float scale = 1.0f)
{
    std::string response;
    if (!HTTPSClient::GET(host, path, response))
    {
        LOGE("Failed to download image from %s%s", host, path);
        return false;
    }

    LOGI("Downloaded image: %zu bytes", response.size());

    // Set position and scale
    image.SetPositionchatpng(x, y);
    image.SetScalechatpng(scale);

    // Load the image from the downloaded data
    if (!image.LoadFromMemorychatpng(
        reinterpret_cast<const unsigned char*>(response.data()),
        response.size()))
    {
        LOGE("Failed to load image from downloaded data");
        return false;
    }

    LOGI("HTTPS image loaded successfully at position (%f, %f) scale %f", x, y, scale);
    return true;
}

// --- Draw Image at World Position ---
void DrawImageAtWorldPosition(RWImagechatpng& image, const CVector& worldPos)
{
    if (!image.texturechatpng || !image.spritechatpng || !pDraw) return;

    float screenX, screenY;
    if (WorldToScreen(worldPos, screenX, screenY))
    {
        // Adjust position for image dimensions
        float adjustedX = screenX - (image.widthchatpng * image.scalechatpng) / 2.0f;
        float adjustedY = screenY - (image.heightchatpng * image.scalechatpng) / 2.0f;
        
        // Store original position
        float originalX = image.xchatpng;
        float originalY = image.ychatpng;
        
        // Temporarily set to world position and draw
        image.SetPositionchatpng(adjustedX, adjustedY);
        CRGBAchatpng color = {255, 255, 255, 255};
        image.Drawchatpng(pDraw, color);
        
        // Restore original position
        image.SetPositionchatpng(originalX, originalY);
    }
}

// --- Hooked functions ---
DECL_HOOKv(CHud_Draw)
{
    CHud_Draw();

    if (pDraw)
    {
        CRGBAchatpng color = {255, 255, 255, 255};
        
        // 1. Draw screen image (normal HUD position)
        if (gImageScreen.texturechatpng && gImageScreen.spritechatpng) 
            gImageScreen.Drawchatpng(pDraw, color);
        
        // 2. Draw image on player's head
        if (gImageHead.texturechatpng && gImageHead.spritechatpng && FindPlayerPed)
        {
            CPed* player = FindPlayerPed();
            if (player)
            {
                CVector headPos = player->vecPosition;
                headPos.z += 1.5f; // Above player's head
                DrawImageAtWorldPosition(gImageHead, headPos);
            }
        }
        
        // 3. Draw image at fixed world position
        if (gImageWorld.texturechatpng && gImageWorld.spritechatpng)
        {
            CVector worldPos = {256.37f, -1392.12f, 11.07f}; // Fixed world coordinates
            DrawImageAtWorldPosition(gImageWorld, worldPos);
        }
    }
}

DECL_HOOKv(CGame_Process)
{
    static bool once = false;
    if (!once)
    {
        HTTPSClient::Initialize();

        // Initialize RW functions first
        InitRwFunctionschatpng(hGTAVC, pGTAVC);

        // Get screen dimensions
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        LOGI("Screen resolution: %dx%d", screenWidth, screenHeight);

        // Load images for different positions:
        
        // 1. Image on screen (HUD position - top-right corner)
        LoadImageFromHTTPS(gImageScreen, "wiki.vc-mp.org", "/images/c/c9/Logo.png", 
                          screenWidth - 100.0f, 50.0f, 0.2f);
        
        // 2. Image for player's head (position will be calculated dynamically)
        LoadImageFromHTTPS(gImageHead, "wiki.vc-mp.org", "/images/c/c9/Logo.png", 
                          0.0f, 0.0f, 0.15f); // Position doesn't matter, will be set dynamically
        
        // 3. Image for fixed world position
        LoadImageFromHTTPS(gImageWorld, "wiki.vc-mp.org", "/images/c/c9/Logo.png", 
                          0.0f, 0.0f, 0.2f); // Position doesn't matter, will be set dynamically

        once = true;
    }
    CGame_Process();
}

// --- Entry point ---
extern "C" void OnModLoad()
{
    LOGI("Mod loading...");

    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (!pGTAVC || !hGTAVC)
    {
        LOGE("Failed to get libGTAVC!");
        return;
    }

    // Get screen resolution functions
    OS_ScreenGetWidth = (OS_ScreenGetWidthFn)aml->GetSym(hGTAVC, "_Z17OS_ScreenGetWidthv");
    OS_ScreenGetHeight = (OS_ScreenGetHeightFn)aml->GetSym(hGTAVC, "_Z18OS_ScreenGetHeightv");
    
    // Get world position functions
    CSprite_CalcScreenCoors = (CSprite_CalcScreenCoorsFn)aml->GetSym(hGTAVC, "_ZN7CSprite15CalcScreenCoorsERK5RwV3dPS0_PfS4_b");
    FindPlayerPed = (FindPlayerPedFn)aml->GetSym(hGTAVC, "_Z13FindPlayerPedv");
    
    if (!OS_ScreenGetWidth || !OS_ScreenGetHeight) {
        LOGI("Screen resolution functions not found, using defaults");
    } else {
        LOGI("Screen resolution functions loaded successfully");
    }
    
    if (!CSprite_CalcScreenCoors || !FindPlayerPed) {
        LOGE("World position functions not found!");
    } else {
        LOGI("World position functions loaded successfully");
    }

    pDraw = (void*)(uintptr_t)aml->GetSym(hGTAVC, "_ZN9CSprite2d4DrawERK5CRectRK5CRGBA");
    if (!pDraw)
    {
        LOGE("Could not find CSprite2d::Draw!");
        return;
    }

    HOOKSYM(CHud_Draw, hGTAVC, "_ZN4CHud4DrawEv");
    HOOKSYM(CGame_Process, hGTAVC, "_ZN5CGame7ProcessEv");

    LOGI("Mod loaded successfully!");
}