#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <stdint.h>
#include <android/log.h>
#include <string.h>
#include <functional>
#include <vector>
#include <includes/scripting.h>

#define LOG_TAG "ViceCityMod"
#define logi(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

MYMODCFG(MEGAMIND.BUTTONS, VCButtons, 1.0, MEGAMIND)

// ---------- STRUCTS ----------
struct CRGBA
{
    unsigned char r, g, b, a;
    CRGBA(unsigned char red=0,unsigned char green=0,unsigned char blue=0,unsigned char alpha=255)
        : r(red), g(green), b(blue), a(alpha) {}
};

struct CRect
{
    float left, top, right, bottom;
    CRect(float l=0,float t=0,float r=0,float b=0):left(l),top(t),right(r),bottom(b){}
    bool Contains(float x,float y) const { return x>=left && x<=right && y>=top && y<=bottom; }
};

struct CVector { float x, y, z; };

struct CPed
{
    void* vtable;
    char pad[0x30];
    CVector position;
};

struct Button
{
    CRect rect;
    CRGBA color;
    const char16_t* text;
    std::function<void()> onClick;
};

// ---------- GLOBALS ----------
std::vector<Button> g_Buttons;
bool g_ButtonsInit = false;

uintptr_t pGTAVC = 0;
void* hGTAVC = nullptr;

// ---------- TYPEDEFS ----------
typedef void (*CSprite2d_DrawRect_t)(const CRect&, const CRGBA&);
typedef void (*CFont_SetScaleFn)(float, float);
typedef void (*CFont_SetColorFn)(CRGBA*);
typedef void (*CFont_SetFontStyleFn)(short);
typedef void (*CFont_SetDropShadowPositionFn)(short);
typedef void (*CFont_SetDropColorFn)(CRGBA*);
typedef void (*CFont_PrintStringFn)(float, float, unsigned short*);
typedef int  (*OS_ScreenGetWidthFn)(void);
typedef int  (*OS_ScreenGetHeightFn)(void);

// scripting + pools
typedef CPed* (*FindPlayerPedFn)(void);
typedef int   (*CPools_GetPedRefFn)(CPed*);
typedef CPed* (*CPools_GetPedFn)(int);

// ---------- POINTERS ----------
CSprite2d_DrawRect_t CSprite2d_DrawRect = nullptr;
CFont_SetScaleFn CFont_SetScale = nullptr;
CFont_SetColorFn CFont_SetColor = nullptr;
CFont_SetFontStyleFn CFont_SetFontStyle = nullptr;
CFont_SetDropShadowPositionFn CFont_SetDropShadowPosition = nullptr;
CFont_SetDropColorFn CFont_SetDropColor = nullptr;
CFont_PrintStringFn CFont_PrintString = nullptr;
OS_ScreenGetWidthFn OS_ScreenGetWidth = nullptr;
OS_ScreenGetHeightFn OS_ScreenGetHeight = nullptr;

FindPlayerPedFn FindPlayerPed = nullptr;
CPools_GetPedRefFn CPools_GetPedRef = nullptr;
CPools_GetPedFn CPools_GetPed = nullptr;

// ---------- BUTTON HELPERS ----------
Button MakeButton(float xPerc, float yPerc, float wPerc, float hPerc,
                  const char16_t* text, std::function<void()> onClick, CRGBA color)
{
    int screenW = OS_ScreenGetWidth();
    int screenH = OS_ScreenGetHeight();

    float left   = screenW * xPerc;
    float top    = screenH * yPerc;
    float right  = left + (screenW * wPerc);
    float bottom = top  + (screenH * hPerc);

    return { CRect(left, top, right, bottom), color, text, onClick };
}

void AddButton(float xPerc, float yPerc, float wPerc, float hPerc,
               const char16_t* text, std::function<void()> onClick, CRGBA color)
{
    g_Buttons.push_back(MakeButton(xPerc, yPerc, wPerc, hPerc, text, onClick, color));
}

// ---------- HANDLE VALIDATION ----------
int GetValidPedHandle(CPed* ped)
{
    if(!ped || !CPools_GetPedRef || !CPools_GetPed) return -1;

    int handle = CPools_GetPedRef(ped);
    if(handle == -1) return -1;

    CPed* back = CPools_GetPed(handle);
    if(back != ped) 
    {
        loge("Handle validation failed: handle=%d back=%p expected=%p", handle, back, ped);
        return -1;
    }

    logi("Handle validated: %d for ped %p", handle, ped);
    return handle;
}

// ---------- CROUCH BUTTON ----------
bool g_IsCrouching = false;

void AddCrouchButton()
{
    AddButton(0.8f, 0.45f, 0.15f, 0.08f, u"Crouch", []()
    {
        CPed* player = FindPlayerPed ? FindPlayerPed() : nullptr;
        if(player)
        {
            int handle = GetValidPedHandle(player);
            if(handle != -1)
            {
                g_IsCrouching = !g_IsCrouching;

                if(g_IsCrouching)
                {
                    ScriptCommand(&scm_actor_crouch_ms, handle, 1, 9999999);
                    logi("Crouching player via ScriptCommand");
                }
                else
                {
                    ScriptCommand(&scm_actor_crouch_ms, handle, 0, 0);
                    logi("Standing player via ScriptCommand");
                }
            }
        }
    }, CRGBA(0,0,0,180));
}

// ---------- HOOK TOUCH ----------
DECL_HOOKv(AND_TouchEvent, int eventType, int pointerId, int x, int y)
{
    if(eventType == 2) // ACTION_DOWN
    {
        for(auto& btn : g_Buttons)
        {
            if(btn.rect.Contains((float)x,(float)y) && btn.onClick) 
                btn.onClick();
        }
    }
    AND_TouchEvent(eventType, pointerId, x, y);
}

// ---------- HOOK HUD DRAW ----------
DECL_HOOKv(CHud_Draw)
{
    CHud_Draw();

    if(!CSprite2d_DrawRect || !CFont_PrintString) return;

    if(!g_ButtonsInit)
    {
        AddCrouchButton();
        g_ButtonsInit = true;
    }

    // ---- Draw All Buttons ----
    for(auto& btn : g_Buttons)
    {
        // Draw background rectangle
        CSprite2d_DrawRect(btn.rect, btn.color);

        float btnW = btn.rect.right - btn.rect.left;
        float btnH = btn.rect.bottom - btn.rect.top;

        float scale = 0.9f;
        CFont_SetScale(scale, scale);

        { CRGBA white(255,255,255,255); CFont_SetColor(&white); }
        if(CFont_SetDropShadowPosition) CFont_SetDropShadowPosition(1);
        if(CFont_SetDropColor){ CRGBA shadow(0,0,0,255); CFont_SetDropColor(&shadow); }

        float textX = btn.rect.left + btnW * 0.5f;
        float textY = btn.rect.top + btnH * 0.5f;

        uintptr_t symAddr = aml->GetSym(hGTAVC, "_ZN5CFont13SetCentreSizeEf");
        if(symAddr)
        {
            typedef void (*CFont_SetCentreSizeFn)(float);
            CFont_SetCentreSizeFn CFont_SetCentreSize = reinterpret_cast<CFont_SetCentreSizeFn>(symAddr);
            CFont_SetCentreSize(btnW * 0.8f);
        }

        CFont_PrintString(textX, textY, (unsigned short*)btn.text);
    }
}

// ---------- MOD ENTRY ----------
extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    InitializeScripting();

    // Grab symbols
    CSprite2d_DrawRect = (CSprite2d_DrawRect_t)aml->GetSym(hGTAVC,"_ZN9CSprite2d8DrawRectERK5CRectRK5CRGBA");
    CFont_SetScale = (CFont_SetScaleFn)aml->GetSym(hGTAVC,"_ZN5CFont8SetScaleEff");
    CFont_PrintString = (CFont_PrintStringFn)aml->GetSym(hGTAVC,"_ZN5CFont11PrintStringEffPt");
    CFont_SetFontStyle = (CFont_SetFontStyleFn)aml->GetSym(hGTAVC,"_ZN5CFont12SetFontStyleEs");
    CFont_SetColor = (CFont_SetColorFn)aml->GetSym(hGTAVC,"_ZN5CFont8SetColorE5CRGBA");
    CFont_SetDropShadowPosition = (CFont_SetDropShadowPositionFn)aml->GetSym(hGTAVC,"_ZN5CFont21SetDropShadowPositionEs");
    CFont_SetDropColor = (CFont_SetDropColorFn)aml->GetSym(hGTAVC,"_ZN5CFont12SetDropColorE5CRGBA");
    OS_ScreenGetWidth = (OS_ScreenGetWidthFn)aml->GetSym(hGTAVC,"_Z17OS_ScreenGetWidthv");
    OS_ScreenGetHeight = (OS_ScreenGetHeightFn)aml->GetSym(hGTAVC,"_Z18OS_ScreenGetHeightv");

    // player + pools
    FindPlayerPed   = (FindPlayerPedFn)aml->GetSym(hGTAVC, "_Z13FindPlayerPedv");
    CPools_GetPedRef= (CPools_GetPedRefFn)aml->GetSym(hGTAVC, "_ZN6CPools9GetPedRefEP4CPed");
    CPools_GetPed   = (CPools_GetPedFn)aml->GetSym(hGTAVC, "_ZN6CPools6GetPedEi");

    // Hooks
    HOOKSYM(CHud_Draw, hGTAVC, "_ZN4CHud4DrawEv");
    HOOKSYM(AND_TouchEvent, hGTAVC, "_Z14AND_TouchEventiiii");

    logi("Vice City Mod loaded: buttons + crouch ready.");
}
