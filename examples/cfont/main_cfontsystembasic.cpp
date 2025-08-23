#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>

MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

struct alignas(4) CRGBA
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    CRGBA(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255)
        : r(_r), g(_g), b(_b), a(_a) {}
    CRGBA() : r(0), g(0), b(0), a(255) {}
};

// Function typedefs
typedef void (*CFont_SetScaleFn)(float, float);
typedef void (*CFont_PrintStringFn)(float, float, unsigned short*);
typedef void (*CFont_SetFontStyleFn)(short);
typedef void (*CFont_SetColorFn)(CRGBA*);
typedef void (*CFont_SetJustifyOnFn)(void);
typedef void (*CFont_SetDropShadowPositionFn)(short);
typedef void (*CFont_SetDropColorFn)(CRGBA*);
typedef void (*VoidFn)(void);

// Font texture loaders
VoidFn CFont_AddEFIGSFont = nullptr;

// Function pointers
CFont_SetScaleFn CFont_SetScale = nullptr;
CFont_PrintStringFn CFont_PrintString = nullptr;
CFont_SetFontStyleFn CFont_SetFontStyle = nullptr;
CFont_SetColorFn CFont_SetColor = nullptr;
CFont_SetJustifyOnFn CFont_SetJustifyOn = nullptr;
CFont_SetDropShadowPositionFn CFont_SetDropShadowPosition = nullptr;
CFont_SetDropColorFn CFont_SetDropColor = nullptr;

// Hook CHud::Draw
DECL_HOOKv(CHud_Draw)
{
    CHud_Draw(); // call original HUD draw

    // Font setup
    if (CFont_SetFontStyle) CFont_SetFontStyle(1);
    if (CFont_SetScale) CFont_SetScale(1.0f, 1.2f);
    if (CFont_SetJustifyOn) CFont_SetJustifyOn();

    CRGBA textColor(135, 206, 250, 255);
    if (CFont_SetColor) CFont_SetColor(&textColor);

    if (CFont_SetDropShadowPosition) CFont_SetDropShadowPosition(1);
    CRGBA shadowColor(0, 0, 0, 255);
    if (CFont_SetDropColor) CFont_SetDropColor(&shadowColor);

    if (CFont_AddEFIGSFont) CFont_AddEFIGSFont();

    float x = 50.0f;

    // Manual lines
    if (CFont_PrintString)
    {
        // Messages
char16_t line0[] = u"MEGAMIND loves modding games.";
char16_t line1[] = u"Vice City is amazing at night.";
char16_t line2[] = u"Random sentence generator in action.";
char16_t line3[] = u"Mods can make games fun again.";
char16_t line4[] = u"Learning C++ is rewarding.";
char16_t line5[] = u"Custom fonts are cool.";
char16_t line6[] = u"Text shadows add depth.";
char16_t line7[] = u"Android mods are popular.";
char16_t line8[] = u"Fonts support multiple languages.";
char16_t line9[] = u"Rendering text is tricky.";
char16_t line10[] = u"Another line to test rendering.";
char16_t line11[] = u"Keep adding more lines here.";
char16_t line12[] = u"Line 13 shows up nicely.";
char16_t line13[] = u"Almost at the bottom of the screen.";
char16_t line14[] = u"Line 15 is just for testing.";
char16_t line15[] = u"Line 16 displays without issues.";
char16_t line16[] = u"Line 17 makes the HUD fuller.";
char16_t line17[] = u"Line 18 is right above the last lines.";
char16_t line18[] = u"Line 19 appears correctly.";
char16_t line19[] = u"Line 20 completes the test.";

// Y positions (20 units apart)
CFont_PrintString(x, 5.0f, reinterpret_cast<unsigned short*>(line0));
CFont_PrintString(x, 25.0f, reinterpret_cast<unsigned short*>(line1));
CFont_PrintString(x, 45.0f, reinterpret_cast<unsigned short*>(line2));
CFont_PrintString(x, 65.0f, reinterpret_cast<unsigned short*>(line3));
CFont_PrintString(x, 85.0f, reinterpret_cast<unsigned short*>(line4));
CFont_PrintString(x, 105.0f, reinterpret_cast<unsigned short*>(line5));
CFont_PrintString(x, 125.0f, reinterpret_cast<unsigned short*>(line6));
CFont_PrintString(x, 145.0f, reinterpret_cast<unsigned short*>(line7));
CFont_PrintString(x, 165.0f, reinterpret_cast<unsigned short*>(line8));
CFont_PrintString(x, 185.0f, reinterpret_cast<unsigned short*>(line9));
CFont_PrintString(x, 205.0f, reinterpret_cast<unsigned short*>(line10));
CFont_PrintString(x, 225.0f, reinterpret_cast<unsigned short*>(line11));
CFont_PrintString(x, 245.0f, reinterpret_cast<unsigned short*>(line12));
CFont_PrintString(x, 265.0f, reinterpret_cast<unsigned short*>(line13));
CFont_PrintString(x, 285.0f, reinterpret_cast<unsigned short*>(line14));
CFont_PrintString(x, 305.0f, reinterpret_cast<unsigned short*>(line15));
CFont_PrintString(x, 325.0f, reinterpret_cast<unsigned short*>(line16));
CFont_PrintString(x, 345.0f, reinterpret_cast<unsigned short*>(line17));
CFont_PrintString(x, 365.0f, reinterpret_cast<unsigned short*>(line18));
CFont_PrintString(x, 385.0f, reinterpret_cast<unsigned short*>(line19));

    }
}

extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (pGTAVC && hGTAVC)
    {
        CFont_SetScale = (CFont_SetScaleFn)aml->GetSym(hGTAVC, "_ZN5CFont8SetScaleEff");
        CFont_PrintString = (CFont_PrintStringFn)aml->GetSym(hGTAVC, "_ZN5CFont11PrintStringEffPt");
        CFont_SetFontStyle = (CFont_SetFontStyleFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetFontStyleEs");
        CFont_SetColor = (CFont_SetColorFn)aml->GetSym(hGTAVC, "_ZN5CFont8SetColorE5CRGBA");
        CFont_SetJustifyOn = (CFont_SetJustifyOnFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetJustifyOnEv");
        CFont_SetDropShadowPosition = (CFont_SetDropShadowPositionFn)aml->GetSym(hGTAVC, "_ZN5CFont21SetDropShadowPositionEs");
        CFont_SetDropColor = (CFont_SetDropColorFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetDropColorE5CRGBA");
        CFont_AddEFIGSFont = (VoidFn)aml->GetSym(hGTAVC, "_ZN5CFont12AddEFIGSFontEv");

        // Hook CHud::Draw instead of CGame::Process
        HOOKSYM(CHud_Draw, hGTAVC, "_ZN4CHud4DrawEv");
    }
}
