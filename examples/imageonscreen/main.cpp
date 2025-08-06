#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <android/log.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "image.h"        // Include image data header
#include "rwimage.h"

#define LOG_TAG "vc_mp"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

// Game function pointers
void* pDraw = nullptr;
void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

// Use CORRECT type (RWImagechatpng) and EMBEDDED DATA
RWImagechatpng image1 = {
    chatKeyboardImageData,   // From image.h
    chatKeyboardImageSize,   // From image.h
    0.0f,                    // X position
    0.0f,                    // Y position
    1.0f                     // Scale factor
};

DECL_HOOKv(CHud_Draw)
{
    CHud_Draw();
    
    // Use CORRECT color type (CRGBAchatpng)
    CRGBAchatpng color = {255, 255, 255, 255};
    image1.Drawchatpng(pDraw, color);
}

DECL_HOOKv(CGame_Process)
{
    static bool once = false;
    if (!once)
    {
        image1.Loadchatpng();  // Use correct function name
        once = true;
    }
    CGame_Process();
}

extern "C" void OnModLoad()
{
    LOGI("Mod loading...");
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (!pGTAVC || !hGTAVC) {
        LOGE("Failed to get libGTAVC!");
        return;
    }

    // Use CORRECT function name
    InitRwFunctionschatpng(hGTAVC, pGTAVC);

    pDraw = (void*)(uintptr_t)aml->GetSym(hGTAVC, "_ZN9CSprite2d4DrawERK5CRectRK5CRGBA");
    if (!pDraw) {
        LOGE("Could not find CSprite2d::Draw!");
        return;
    }

    HOOKSYM(CHud_Draw, hGTAVC, "_ZN4CHud4DrawEv");
    HOOKSYM(CGame_Process, hGTAVC, "_ZN5CGame7ProcessEv");
    LOGI("Mod loaded successfully!");
}