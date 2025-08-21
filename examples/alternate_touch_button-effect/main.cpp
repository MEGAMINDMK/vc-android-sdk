#include <mod/amlmod.h>    // AMLMod core
#include <mod/logger.h>    // Logging utilities
#include <mod/config.h>    // Config management
#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>
#include <android/log.h>

#define LOG_TAG "vcmp"
#define logi(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Define mod metadata
MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

// Global pointers to GTA:VC library and base
void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

// RGBA color struct (4-byte aligned)
struct alignas(4) CRGBA
{
    uint8_t r, g, b, a;

    CRGBA(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255)
        : r(_r), g(_g), b(_b), a(_a) {}
    CRGBA() : r(0), g(0), b(0), a(255) {}
};

// -------------------- Function typedefs --------------------
typedef void (*CFont_SetScaleFn)(float, float);
typedef void (*CFont_PrintStringFn)(float, float, unsigned short*);
typedef void (*CFont_SetFontStyleFn)(short);
typedef void (*CFont_SetColorFn)(CRGBA*);
typedef void (*CFont_SetJustifyOnFn)(void);
typedef void (*CFont_SetDropShadowPositionFn)(short);
typedef void (*CFont_SetDropColorFn)(CRGBA*);
typedef void (*VoidFn)(void);
typedef int (*OS_ScreenGetWidthFn)(void);
typedef int (*OS_ScreenGetHeightFn)(void);

// -------------------- Function pointers --------------------
// Font texture loaders
VoidFn CFont_AddEFIGSFont = nullptr;
VoidFn CFont_AddJapaneseTexture = nullptr;
VoidFn CFont_AddRussianTexture = nullptr;
VoidFn CFont_AddKoreanTexture = nullptr;

// Font functions
CFont_SetScaleFn CFont_SetScale = nullptr;
CFont_PrintStringFn CFont_PrintString = nullptr;
CFont_SetFontStyleFn CFont_SetFontStyle = nullptr;
CFont_SetColorFn CFont_SetColor = nullptr;
CFont_SetJustifyOnFn CFont_SetJustifyOn = nullptr;
CFont_SetDropShadowPositionFn CFont_SetDropShadowPosition = nullptr;
CFont_SetDropColorFn CFont_SetDropColor = nullptr;

// Screen functions
OS_ScreenGetWidthFn OS_ScreenGetWidth = nullptr;
OS_ScreenGetHeightFn OS_ScreenGetHeight = nullptr;

// -------------------- Global state --------------------
static CRGBA g_TextColor(135, 206, 250, 255); // default light blue
static float g_TextX = 0.0f, g_TextY = 0.0f;
static float g_ScreenW = 0.0f, g_ScreenH = 0.0f;

// -------------------- Hook touch event --------------------
DECL_HOOKv(AND_TouchEvent, int eventType, int pointerId, int x, int y)
{
    logi("Touch Event: type=%d, id=%d, x=%d, y=%d", eventType, pointerId, x, y);

    // Only handle ACTION_DOWN (2) for tap detection
    if(eventType == 2)
    {
        // Simple hitbox around the text (150x50 rectangle)
        float halfW = 150.0f, halfH = 50.0f;
        if(x >= g_TextX-halfW && x <= g_TextX+halfW &&
           y >= g_TextY-halfH && y <= g_TextY+halfH)
        {
            // Change text color randomly when tapped
            g_TextColor = CRGBA(rand()%256, rand()%256, rand()%256, 255);
            logi("Text color changed!");
        }
    }

    // Call original function
    AND_TouchEvent(eventType, pointerId, x, y);
}

// -------------------- Hook game loop --------------------
DECL_HOOKv(CGame_Process)
{
    // Call original game logic
    CGame_Process();

    // Setup font properties
    if (CFont_SetFontStyle) CFont_SetFontStyle(1);   // Normal font style
    if (CFont_SetScale) CFont_SetScale(1.0f, 1.2f);  // Scale X/Y
    if (CFont_SetJustifyOn) CFont_SetJustifyOn();    // Left-align

    // Apply global text color
    if (CFont_SetColor) CFont_SetColor(&g_TextColor);

    // Set drop shadow
    if (CFont_SetDropShadowPosition) CFont_SetDropShadowPosition(1);
    CRGBA shadowColor(0, 0, 0, 255);
    if (CFont_SetDropColor) CFont_SetDropColor(&shadowColor);

    // Load English font textures
    if (CFont_AddEFIGSFont) CFont_AddEFIGSFont();

    // Get dynamic screen width and height
    g_ScreenW = (float)OS_ScreenGetWidth();
    g_ScreenH = (float)OS_ScreenGetHeight();

    g_TextX = g_ScreenW / 2.0f;
    g_TextY = g_ScreenH / 2.0f;

    // Print text on screen
    if (CFont_PrintString)
    {
        char16_t msg[] = u"Tap me to change my color!";
        CFont_PrintString(g_TextX, g_TextY, reinterpret_cast<unsigned short*>(msg));
    }
}

// -------------------- OnModLoad --------------------
extern "C" void OnModLoad()
{
    // Get library handle
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (!pGTAVC || !hGTAVC) return;

    // Load font-related symbols
    CFont_SetScale = (CFont_SetScaleFn)aml->GetSym(hGTAVC, "_ZN5CFont8SetScaleEff");
    CFont_PrintString = (CFont_PrintStringFn)aml->GetSym(hGTAVC, "_ZN5CFont11PrintStringEffPt");
    CFont_SetFontStyle = (CFont_SetFontStyleFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetFontStyleEs");
    CFont_SetColor = (CFont_SetColorFn)aml->GetSym(hGTAVC, "_ZN5CFont8SetColorE5CRGBA");
    CFont_SetJustifyOn = (CFont_SetJustifyOnFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetJustifyOnEv");
    CFont_SetDropShadowPosition = (CFont_SetDropShadowPositionFn)aml->GetSym(hGTAVC, "_ZN5CFont21SetDropShadowPositionEs");
    CFont_SetDropColor = (CFont_SetDropColorFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetDropColorE5CRGBA");

    // Font texture loaders
    CFont_AddEFIGSFont = (VoidFn)aml->GetSym(hGTAVC, "_ZN5CFont12AddEFIGSFontEv");
    CFont_AddJapaneseTexture = (VoidFn)aml->GetSym(hGTAVC, "_ZN5CFont18AddJapaneseTextureEv");
    CFont_AddRussianTexture = (VoidFn)aml->GetSym(hGTAVC, "_ZN5CFont17AddRussianTextureEv");
    CFont_AddKoreanTexture = (VoidFn)aml->GetSym(hGTAVC, "_ZN5CFont16AddKoreanTextureEv");

    // Screen functions
    OS_ScreenGetWidth = (OS_ScreenGetWidthFn)aml->GetSym(hGTAVC, "_Z17OS_ScreenGetWidthv");
    OS_ScreenGetHeight = (OS_ScreenGetHeightFn)aml->GetSym(hGTAVC, "_Z18OS_ScreenGetHeightv");

    // Hooks
    HOOKSYM(CGame_Process, hGTAVC, "_ZN5CGame7ProcessEv");
    HOOKSYM(AND_TouchEvent, hGTAVC, "_Z14AND_TouchEventiiii");
}
