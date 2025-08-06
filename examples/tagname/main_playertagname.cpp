#include <mod/amlmod.h>
#include <mod/logger.h>
#include <stdint.h>
#include <string.h>
#include <mod/config.h>
#include <android/log.h>

// --- Logging Macros ---
#define LOG_TAG "ViceCityMod"
#define logi(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// --- Mod Info ---
MYMODCFG(MEGAMIND.VCMOD, ViceCityHeadName, 1.1, MEGAMIND)

// --- Base Definitions ---
void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

// --- Structures ---
struct alignas(4) CRGBA
{
    uint8_t r, g, b, a;
    CRGBA(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255)
        : r(_r), g(_g), b(_b), a(_a) {}
    CRGBA() : r(0), g(0), b(0), a(255) {}
};

struct CVector
{
    float x, y, z;
};

struct RwV3d
{
    float x, y, z;
};

// Updated CPed struct with correct padding
struct CPed
{
    void* vtable;
    char pad[0x30];
    CVector vecPosition;
};

// --- Function Typedefs ---
typedef void (*CFont_SetScaleFn)(float, float);
typedef void (*CFont_PrintStringFn)(float, float, unsigned short*);
typedef void (*CFont_SetFontStyleFn)(short);
typedef void (*CFont_SetColorFn)(CRGBA*);
typedef void (*CFont_SetDropShadowPositionFn)(short);
typedef void (*CFont_SetDropColorFn)(CRGBA*);
typedef void (*CFont_SetJustifyOnFn)(void);
typedef bool (*CSprite_CalcScreenCoorsFn)(const RwV3d&, RwV3d*, float*, float*, bool);
typedef CPed* (*FindPlayerPedFn)();
typedef CPed* (*GetPedFn)(int);

// --- Pointers to Game Functions ---
CFont_SetScaleFn CFont_SetScale = nullptr;
CFont_PrintStringFn CFont_PrintString = nullptr;
CFont_SetFontStyleFn CFont_SetFontStyle = nullptr;
CFont_SetColorFn CFont_SetColor = nullptr;
CFont_SetDropShadowPositionFn CFont_SetDropShadowPosition = nullptr;
CFont_SetDropColorFn CFont_SetDropColor = nullptr;
CFont_SetJustifyOnFn CFont_SetJustifyOn = nullptr;
CSprite_CalcScreenCoorsFn CSprite_CalcScreenCoors = nullptr;
FindPlayerPedFn FindPlayerPed = nullptr;
GetPedFn GetPed = nullptr;
CPedPool** ms_pPedPool = nullptr;

// --- WorldToScreen Helper ---
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

// --- Hook: CHud::Draw ---
DECL_HOOKv(CHud_Draw)
{
    CHud_Draw(); // Original draw

    if (!FindPlayerPed || !CFont_PrintString) return;

    CPed* player = FindPlayerPed();
    if (!player) return;

    CVector pos = player->vecPosition;
    pos.z += 1.0f; // Slightly above head

    float screenX, screenY;
    if (WorldToScreen(pos, screenX, screenY))
    {
        // Font settings
        CFont_SetFontStyle(1); // FONT_SUBTITLES
        CFont_SetScale(0.6f, 0.6f);
        CFont_SetJustifyOn();

        CRGBA color(255, 255, 255, 255);
        CRGBA shadow(0, 0, 0, 255);
        CFont_SetColor(&color);
        CFont_SetDropShadowPosition(1);
        CFont_SetDropColor(&shadow);

        char16_t text[] = u"PlayerName";
        CFont_PrintString(screenX, screenY, (unsigned short*)text);
    }

    // Optional: debug log
    logi("Player Pos: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
}

// --- Hook: CGame::Process (optional) ---
DECL_HOOKv(CGame_Process)
{
    CGame_Process(); // Original
    // Add extra logic if needed
}

// --- Entry Point ---
extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");
    if (!pGTAVC || !hGTAVC) return;

    // Log loaded
    logi("ViceCityHeadName mod loaded!");

    // Load symbols
    CFont_SetScale = (CFont_SetScaleFn)aml->GetSym(hGTAVC, "_ZN5CFont8SetScaleEff");
    CFont_PrintString = (CFont_PrintStringFn)aml->GetSym(hGTAVC, "_ZN5CFont11PrintStringEffPt");
    CFont_SetFontStyle = (CFont_SetFontStyleFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetFontStyleEs");
    CFont_SetColor = (CFont_SetColorFn)aml->GetSym(hGTAVC, "_ZN5CFont8SetColorE5CRGBA");
    CFont_SetDropShadowPosition = (CFont_SetDropShadowPositionFn)aml->GetSym(hGTAVC, "_ZN5CFont21SetDropShadowPositionEs");
    CFont_SetDropColor = (CFont_SetDropColorFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetDropColorE5CRGBA");
    CFont_SetJustifyOn = (CFont_SetJustifyOnFn)aml->GetSym(hGTAVC, "_ZN5CFont12SetJustifyOnEv");

    CSprite_CalcScreenCoors = (CSprite_CalcScreenCoorsFn)aml->GetSym(hGTAVC, "_ZN7CSprite15CalcScreenCoorsERK5RwV3dPS0_PfS4_b");
    FindPlayerPed = (FindPlayerPedFn)aml->GetSym(hGTAVC, "_Z13FindPlayerPedv");

   // Ped pool pointers
    ms_pPedPool = (CPedPool**)aml->GetSym(hGTAVC, "_ZN6CPools10ms_pPedPoolE");
    GetPed = (GetPedFn)aml->GetSym(hGTAVC, "_ZN6CPools7GetPedEi");
    // Hook into game
    HOOKSYM(CGame_Process, hGTAVC, "_ZN5CGame7ProcessEv");
    HOOKSYM(CHud_Draw, hGTAVC, "_ZN4CHud4DrawEv");
}