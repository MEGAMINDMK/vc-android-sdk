#include <jni.h>
#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>
#include <string>
#include <android/log.h>
#include <cinttypes>
#include <chrono>

#define LOG_TAG "VCMP"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace std::chrono;

MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

// ======================= CRGBA =======================
struct alignas(4) CRGBA
{
    uint8_t r, g, b, a;
    CRGBA(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255) : r(_r), g(_g), b(_b), a(_a) {}
    CRGBA() : r(0), g(0), b(0), a(255) {}
};

// ======================= Font typedefs =======================
typedef void (*CFont_SetScaleFn)(float, float);
typedef void (*CFont_PrintStringFn)(float, float, unsigned short*);
typedef void (*CFont_SetFontStyleFn)(short);
typedef void (*CFont_SetColorFn)(CRGBA*);
typedef void (*CFont_SetJustifyOnFn)(void);
typedef void (*CFont_SetDropShadowPositionFn)(short);
typedef void (*CFont_SetDropColorFn)(CRGBA*);
typedef void (*VoidFn)(void);

VoidFn CFont_AddFont = nullptr;
CFont_SetScaleFn CFont_SetScale = nullptr;
CFont_PrintStringFn CFont_PrintString = nullptr;
CFont_SetFontStyleFn CFont_SetFontStyle = nullptr;
CFont_SetColorFn CFont_SetColor = nullptr;
CFont_SetJustifyOnFn CFont_SetJustifyOn = nullptr;
CFont_SetDropShadowPositionFn CFont_SetDropShadowPosition = nullptr;
CFont_SetDropColorFn CFont_SetDropColor = nullptr;

// ======================= OS Keyboard =======================
typedef void (*OS_KeyboardRequestFn)(int);
OS_KeyboardRequestFn OS_KeyboardRequest = nullptr;

// ======================= Typed message =======================
std::u16string currentMessage = u"";

// ======================= Debounce state =======================
static char lastChar = 0;
static steady_clock::time_point lastCharTime;


// ======================= Hook AND_KeyboardGetChar =======================
DECL_HOOK(char, AND_KeyboardGetChar, void* osKey, int someInt)
{
    char c = AND_KeyboardGetChar(osKey, someInt);

    if (c != 0)
    {
        // Debounce ONLY for normal characters
        auto now = steady_clock::now();
        auto ms = duration_cast<milliseconds>(now - lastCharTime).count();

        if (c != lastChar || ms > 150) // allow same char only after 150ms
        {
            currentMessage += static_cast<char16_t>(c);
            lastChar = c;
            lastCharTime = now;

            LOGD("Typed char: %c | Current message: %ls", c, reinterpret_cast<const wchar_t*>(currentMessage.c_str()));
        }
    }

    return c;
}

// ======================= Hook KeyboardHandler for Backspace/Enter/Space =======================
DECL_HOOK(int, KeyboardHandler, int event, void* param)
{
    if (param && event == 28)
    {
        int keycode = *(int*)param;

        if (keycode == 1042 && !currentMessage.empty()) // Backspace (no debounce)
            currentMessage.pop_back();
        else if (keycode == 1045) // Enter (no debounce)
            currentMessage += u"";
        else if (keycode == 32) // Space (no debounce)
            currentMessage += u" ";
    }

    return KeyboardHandler(event, param);
}


// ======================= Hook AND_TouchEvent =======================
DECL_HOOK(int, AND_TouchEvent, int eventType, int x, int y, int pointerId)
{
    if (eventType == 2) // touch down
    {
        // Request soft keyboard on any touch anywhere
        if (OS_KeyboardRequest)
            OS_KeyboardRequest(1);
    }

    return AND_TouchEvent(eventType, x, y, pointerId);
}

// ======================= Hook CHud::Draw to render typed text =======================
DECL_HOOKv(CHud_Draw)
{
    CHud_Draw(); // original HUD draw

    if (!CFont_PrintString) return;

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
    float y = 50.0f;

    // Split currentMessage into lines if it contains \n
    size_t start = 0;
    size_t lineIndex = 0;
    while (start < currentMessage.size())
    {
        size_t end = currentMessage.find(u'\n', start);
        if (end == std::u16string::npos) end = currentMessage.size();

        std::u16string line = currentMessage.substr(start, end - start);
        CFont_PrintString(x, y + lineIndex * 20.0f, reinterpret_cast<unsigned short*>(line.data()));

        start = end + 1;
        lineIndex++;
    }
}

// ======================= Mod load =======================
extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (pGTAVC && hGTAVC)
    {
        // Font symbols
        CFont_SetScale = (CFont_SetScaleFn)aml->GetSym(hGTAVC, "_ZN5CFont8SetScaleEff");
        CFont_PrintString = (CFont_PrintStringFn)aml->GetSym(hGTAVC, "_ZN5CFont11PrintStringEffPt");
        CFont_SetFontStyle = (CFont_SetFontStyleFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetFontStyleEs");
        CFont_SetColor = (CFont_SetColorFn)aml->GetSym(hGTAVC, "_ZN5CFont8SetColorE5CRGBA");
        CFont_SetJustifyOn = (CFont_SetJustifyOnFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetJustifyOnEv");
        CFont_SetDropShadowPosition = (CFont_SetDropShadowPositionFn)aml->GetSym(hGTAVC, "_ZN5CFont21SetDropShadowPositionEs");
        CFont_SetDropColor = (CFont_SetDropColorFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetDropColorE5CRGBA");
        CFont_AddFont = (VoidFn)aml->GetSym(hGTAVC, "_ZN5CFont18AddJapaneseTextureEv");

        // OS Keyboard
        OS_KeyboardRequest = (OS_KeyboardRequestFn)aml->GetSym(hGTAVC, "_Z18OS_KeyboardRequesti");

        // Hooks
        HOOKSYM(CHud_Draw, hGTAVC, "_ZN4CHud4DrawEv");
        HOOKSYM(KeyboardHandler, hGTAVC, "_Z15KeyboardHandler7RsEventPv");
        HOOKSYM(AND_KeyboardGetChar, hGTAVC, "_Z19AND_KeyboardGetChar13OSKeyboardKeyi");
        HOOKSYM(AND_TouchEvent, hGTAVC, "_Z14AND_TouchEventiiii");

        LOGD("Full screen touch + soft keyboard hooks installed successfully!");
    }
    else
    {
        LOGE("Failed to load GTA VC library!");
    }
}
