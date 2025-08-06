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

struct CRGBA
{
    uint8_t r, g, b, a;
    CRGBA(uint8_t _r = 0, uint8_t _g = 0, uint8_t _b = 0, uint8_t _a = 255)
        : r(_r), g(_g), b(_b), a(_a) {}
};

static CRGBA ParseHexColor(const std::string& hex)
{
    if(hex.length() != 7 || hex[0] != '#') return CRGBA(255, 255, 255);
    int r = std::stoi(hex.substr(1, 2), nullptr, 16);
    int g = std::stoi(hex.substr(3, 2), nullptr, 16);
    int b = std::stoi(hex.substr(5, 2), nullptr, 16);
    return CRGBA(r, g, b, 255);
}

void RenderColoredText(const std::string& input, float startX, float y,
                       void (*SetColor)(CRGBA*),
                       void (*PrintString)(float, float, unsigned short*))
{
    float x = startX;
    size_t i = 0;
    CRGBA currentColor(255, 255, 255);
    
    while (i < input.size())
    {
        if (input[i] == '[' && i + 8 < input.size() && input[i+1] == '#' && input[i+8] == ']')
        {
            currentColor = ParseHexColor(input.substr(i+1, 7));
            i += 9;
            continue;
        }

        std::string textPart;
        while (i < input.size() && !(input[i] == '[' && i + 8 < input.size() && input[i+1] == '#' && input[i+8] == ']'))
        {
            textPart += input[i++];
        }

        if (!textPart.empty())
        {
            if (SetColor) SetColor(&currentColor);

            std::u16string u16text(textPart.begin(), textPart.end());
            if (PrintString) PrintString(x, y, reinterpret_cast<unsigned short*>(u16text.data()));

            x += textPart.length() * 10.0f; // Adjust spacing for font width
        }
    }
}


// Function typedefs
typedef void (*CFont_SetScaleFn)(float, float);
typedef void (*CFont_PrintStringFn)(float, float, unsigned short*);
typedef void (*CFont_SetFontStyleFn)(short);
typedef void (*CFont_SetColorFn)(CRGBA*);
typedef void (*CFont_SetJustifyOnFn)(void);
typedef void (*CFont_SetDropShadowPositionFn)(short);
typedef void (*CFont_SetDropColorFn)(CRGBA*);
typedef void (*VoidFn)(void);

// Function pointers
CFont_SetScaleFn CFont_SetScale = nullptr;
CFont_PrintStringFn CFont_PrintString = nullptr;
CFont_SetFontStyleFn CFont_SetFontStyle = nullptr;
CFont_SetColorFn CFont_SetColor = nullptr;
CFont_SetJustifyOnFn CFont_SetJustifyOn = nullptr;
CFont_SetDropShadowPositionFn CFont_SetDropShadowPosition = nullptr;
CFont_SetDropColorFn CFont_SetDropColor = nullptr;
VoidFn CFont_AddEFIGSFont = nullptr;

// Hook: CGame::Process
DECL_HOOKv(CGame_Process)
{
    CGame_Process(); // Original game logic

    if (CFont_SetFontStyle) CFont_SetFontStyle(1);
    if (CFont_SetScale) CFont_SetScale(1.0f, 1.2f);
    if (CFont_SetJustifyOn) CFont_SetJustifyOn();

    if (CFont_SetDropShadowPosition) CFont_SetDropShadowPosition(1);
    CRGBA shadowColor(0, 0, 0, 255);
    if (CFont_SetDropColor) CFont_SetDropColor(&shadowColor);

    if (CFont_AddEFIGSFont) CFont_AddEFIGSFont();

    // Colored Text
    std::string input = "[#ff0000]Hello [#00ff00]World[#0000ff] !";
    RenderColoredText(input, 300.0f, 40.0f, CFont_SetColor, CFont_PrintString);
}

// On Mod Load
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

        HOOKSYM(CGame_Process, hGTAVC, "_ZN5CGame7ProcessEv");
    }
}
