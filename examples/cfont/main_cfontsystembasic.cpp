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
VoidFn CFont_AddJapaneseTexture = nullptr;
VoidFn CFont_AddRussianTexture = nullptr;
VoidFn CFont_AddKoreanTexture = nullptr;

// Function pointers
CFont_SetScaleFn CFont_SetScale = nullptr;
CFont_PrintStringFn CFont_PrintString = nullptr;
CFont_SetFontStyleFn CFont_SetFontStyle = nullptr;
CFont_SetColorFn CFont_SetColor = nullptr;
CFont_SetJustifyOnFn CFont_SetJustifyOn = nullptr;
CFont_SetDropShadowPositionFn CFont_SetDropShadowPosition = nullptr;
CFont_SetDropColorFn CFont_SetDropColor = nullptr;

// Hook CGame::Process
DECL_HOOKv(CGame_Process)
{
    CGame_Process(); // Call original game logic

    if (CFont_SetFontStyle) CFont_SetFontStyle(1);
    if (CFont_SetScale) CFont_SetScale(1.0f, 1.2f);
    if (CFont_SetJustifyOn) CFont_SetJustifyOn();

    // Set text color
    CRGBA red(135, 206, 250, 255);
    if (CFont_SetColor) CFont_SetColor(&red);

    // Set drop shadow
    if (CFont_SetDropShadowPosition) CFont_SetDropShadowPosition(1);
    CRGBA shadowColor(0, 0, 0, 255);
    if (CFont_SetDropColor) CFont_SetDropColor(&shadowColor);

    // Print text
	 // EFIGS (English, etc.)
    if (CFont_AddEFIGSFont) CFont_AddEFIGSFont();
    if (CFont_PrintString)
    {
        char16_t msg[] = u"MEGAMIND was alwasy fond of making vcmp android but with his freind gtamods they are trying there best";
        CFont_PrintString(300.0f, 40.0f, reinterpret_cast<unsigned short*>(msg));
    }
	
/*	usage before printing
	if (CFont_AddJapaneseTexture) CFont_AddJapaneseTexture();
	if (CFont_AddRussianTexture) CFont_AddRussianTexture();
	if (CFont_AddKoreanTexture) CFont_AddKoreanTexture();*/
}

extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (pGTAVC && hGTAVC)
    {
        // Load font-related symbols
        CFont_SetScale = (CFont_SetScaleFn)aml->GetSym(hGTAVC, "_ZN5CFont8SetScaleEff");
        CFont_PrintString = (CFont_PrintStringFn)aml->GetSym(hGTAVC, "_ZN5CFont11PrintStringEffPt");
        CFont_SetFontStyle = (CFont_SetFontStyleFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetFontStyleEs");
        CFont_SetColor = (CFont_SetColorFn)aml->GetSym(hGTAVC, "_ZN5CFont8SetColorE5CRGBA");
        CFont_SetJustifyOn = (CFont_SetJustifyOnFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetJustifyOnEv");
        CFont_SetDropShadowPosition = (CFont_SetDropShadowPositionFn)aml->GetSym(hGTAVC, "_ZN5CFont21SetDropShadowPositionEs");
        CFont_SetDropColor = (CFont_SetDropColorFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetDropColorE5CRGBA");
        // Font texture loaders
        CFont_AddEFIGSFont = (VoidFn)aml->GetSym(hGTAVC, "_ZN5CFont12AddEFIGSFontEv");            // CFont::AddEFIGSFont()
        CFont_AddJapaneseTexture = (VoidFn)aml->GetSym(hGTAVC, "_ZN5CFont18AddJapaneseTextureEv");// CFont::AddJapaneseTexture()
        CFont_AddRussianTexture = (VoidFn)aml->GetSym(hGTAVC, "_ZN5CFont17AddRussianTextureEv");  // CFont::AddRussianTexture()
        CFont_AddKoreanTexture = (VoidFn)aml->GetSym(hGTAVC, "_ZN5CFont16AddKoreanTextureEv");    // CFont::AddKoreanTexture()

        // Hook game loop
        HOOKSYM(CGame_Process, hGTAVC, "_ZN5CGame7ProcessEv");
    }
}
