#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <stdint.h>
#include <dlfcn.h>
#include <android/log.h>
#include <string.h>

#define LOG_TAG "ViceCityMod"
#define logi(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

MYMODCFG(MEGAMIND.TXDDRAW, TxdImageDraw, 1.0, MEGAMIND)

// ---------- STRUCTS ----------
struct CRGBA
{
    unsigned char r, g, b, a;
    CRGBA(unsigned char red = 0, unsigned char green = 0, unsigned char blue = 0, unsigned char alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}
};

struct CRect
{
    float left, top, right, bottom;
    CRect(float l = 0.0f, float t = 0.0f, float r = 0.0f, float b = 0.0f)
        : left(l), top(t), right(r), bottom(b) {}
};

// ---------- TYPEDEFS ----------
typedef void (*CSprite2d_DrawRect_t)(const CRect&, const CRGBA&);
typedef void (*CFont_SetScaleFn)(float, float);
typedef void (*CFont_SetColorFn)(CRGBA*);
typedef void (*CFont_SetFontStyleFn)(short);
typedef void (*CFont_SetDropShadowPositionFn)(short);
typedef void (*CFont_SetDropColorFn)(CRGBA*);
typedef void (*CFont_PrintStringFn)(float, float, unsigned short*);

// ---------- POINTERS ----------
CSprite2d_DrawRect_t CSprite2d_DrawRect = nullptr;
CFont_SetScaleFn CFont_SetScale = nullptr;
CFont_SetColorFn CFont_SetColor = nullptr;
CFont_SetFontStyleFn CFont_SetFontStyle = nullptr;
CFont_SetDropShadowPositionFn CFont_SetDropShadowPosition = nullptr;
CFont_SetDropColorFn CFont_SetDropColor = nullptr;
CFont_PrintStringFn CFont_PrintString = nullptr;

// ---------- HOOK: HUD DRAW ----------
DECL_HOOKv(CHud_Draw)
{
    CHud_Draw(); // Original HUD

    if (!CSprite2d_DrawRect) return;

    // ---- Menu Config ----
    float menuX = 100.0f;
    float menuY = 100.0f;
    float menuW = 400.0f;
    float menuH = 300.0f;
    float titleH = 40.0f;
    float optHeight = 40.0f;
    float optMargin = 10.0f;

    // ---- Draw Menu Background ----
    CRect menuBg(menuX, menuY, menuX + menuW, menuY + menuH);
    CRGBA menuBgColor(0, 0, 0, 180);
    CSprite2d_DrawRect(menuBg, menuBgColor);

    // ---- Title Bar ----
    CRect titleBar(menuX, menuY, menuX + menuW, menuY + titleH);
    CRGBA titleColor(255, 165, 0, 255);
    CSprite2d_DrawRect(titleBar, titleColor);

    // ---- Option 1 ----
    float optY1 = menuY + titleH + optMargin;
    CRect option1(menuX + optMargin, optY1, menuX + menuW - optMargin, optY1 + optHeight);
    CRGBA optionColor(50, 50, 50, 255);
    CSprite2d_DrawRect(option1, optionColor);

    // ---- Option 2 ----
    float optY2 = optY1 + optHeight + optMargin;
    CRect option2(menuX + optMargin, optY2, menuX + menuW - optMargin, optY2 + optHeight);
    CSprite2d_DrawRect(option2, optionColor);

    // ---- Draw Text ----
    if (!CFont_PrintString || !CFont_SetFontStyle || !CFont_SetColor || !CFont_SetScale) return;

    CFont_SetFontStyle(1);
    CFont_SetScale(0.6f, 1.0f);
    CRGBA fontColor(255, 255, 255, 255);
    CFont_SetColor(&fontColor);

    if (CFont_SetDropShadowPosition) CFont_SetDropShadowPosition(1);
    if (CFont_SetDropColor)
    {
        CRGBA shadow(0, 0, 0, 255);
        CFont_SetDropColor(&shadow);
    }

    // Title text
    char16_t titleText[] = u"Android Menu";
    CFont_PrintString(menuX + 150.0f, menuY + 10.0f, (unsigned short*)titleText);

    // Option 1 text
    char16_t opt1Text[] = u"Join Server";
    CFont_PrintString(menuX + 80.0f, optY1 + 10.0f, (unsigned short*)opt1Text);

    // Option 2 text
    char16_t opt2Text[] = u"Settings";
    CFont_PrintString(menuX + 60.0f, optY2 + 10.0f, (unsigned short*)opt2Text);
}

// ---------- MOD ENTRY ----------
extern "C" void OnModLoad()
{
    logi("OnModLoad called.");

    uintptr_t pGTAVC = aml->GetLib("libGTAVC.so");
    void* hGTAVC = aml->GetLibHandle("libGTAVC.so");
    logi("libGTAVC.so loaded at: 0x%X", pGTAVC);

    // Load drawing function
    CSprite2d_DrawRect = (CSprite2d_DrawRect_t)aml->GetSym(hGTAVC, "_ZN9CSprite2d8DrawRectERK5CRectRK5CRGBA");

    // Load font functions
    CFont_SetScale = (CFont_SetScaleFn)aml->GetSym(hGTAVC, "_ZN5CFont8SetScaleEff");
    CFont_PrintString = (CFont_PrintStringFn)aml->GetSym(hGTAVC, "_ZN5CFont11PrintStringEffPt");
    CFont_SetFontStyle = (CFont_SetFontStyleFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetFontStyleEs");
    CFont_SetColor = (CFont_SetColorFn)aml->GetSym(hGTAVC, "_ZN5CFont8SetColorE5CRGBA");
    CFont_SetDropShadowPosition = (CFont_SetDropShadowPositionFn)aml->GetSym(hGTAVC, "_ZN5CFont21SetDropShadowPositionEs");
    CFont_SetDropColor = (CFont_SetDropColorFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetDropColorE5CRGBA");

    if (CSprite2d_DrawRect) logi("Resolved: CSprite2d::DrawRect");
    else loge("Failed: CSprite2d::DrawRect");

    // Hook HUD draw
    HOOKSYM(CHud_Draw, hGTAVC, "_ZN4CHud4DrawEv");
    logi("CHud::Draw hooked successfully.");
}