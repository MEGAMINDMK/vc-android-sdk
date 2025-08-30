#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <android/log.h>
#include <stdint.h>
#include "includes/scripting.h"
#include <unistd.h>  // for usleep

#define LOG_TAG "ViceCityMod"
#define logi(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ========== Mod Info ==========
MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

// ========== State ==========
static bool hasSetup = false;

// ---------- Structs ----------
struct CVector { float x,y,z; };
struct CPed
{
    void* vtable;
    char pad[0x30];
    CVector position;
};

// ---------- Typedefs ----------
typedef CPed* (*FindPlayerPedFn)(void);
typedef int   (*CPools_GetPedRefFn)(CPed*);
typedef CPed* (*CPools_GetPedFn)(int handle);

// ---------- Globals ----------
static FindPlayerPedFn    FindPlayerPed    = nullptr;
static CPools_GetPedRefFn CPools_GetPedRef = nullptr;
static CPools_GetPedFn    CPools_GetPed    = nullptr;

// ========== Hook ==========
DECL_HOOKv(CGame_Process)
{
    if(!hasSetup)
    {
        CPed* pPlayer = FindPlayerPed ? FindPlayerPed() : nullptr;
        if(!pPlayer)
        {
            usleep(1000); // sleep 1 ms and retry next frame
        }
        else
        {
            logi("Found player ped at %p", pPlayer);

            if(CPools_GetPedRef && CPools_GetPed)
            {
                int handle = CPools_GetPedRef(pPlayer);
                CPed* backFromHandle = CPools_GetPed(handle);
                logi("Handle for player: %d -> back to ped: %p", handle, backFromHandle);

                if(handle > 0)
                {
                    // ✅ Use handle, not CPed*
                    ScriptCommand(&scm_SET_CHAR_HEALTH, handle, 500); //set health 500 on game start
                    logi("Set player health via ScriptCommand (handle %d)", handle);
                }
                else
                {
                    loge("Invalid handle for player!");
                }
            }

            hasSetup = true;
        }
    }

    CGame_Process(); // continue normal game loop
}

// ========== Entry ==========
void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if(pGTAVC && hGTAVC)
    {
        InitializeScripting();

        // player + pools
        FindPlayerPed    = (FindPlayerPedFn)aml->GetSym(hGTAVC, "_Z13FindPlayerPedv");
        CPools_GetPedRef = (CPools_GetPedRefFn)aml->GetSym(hGTAVC, "_ZN6CPools9GetPedRefEP4CPed");
        CPools_GetPed    = (CPools_GetPedFn)aml->GetSym(hGTAVC, "_ZN6CPools6GetPedEi");

        if(FindPlayerPed)    logi("FindPlayerPed resolved at %p", FindPlayerPed); 
        else                 loge("Failed to resolve FindPlayerPed!");

        if(CPools_GetPedRef) logi("CPools::GetPedRef resolved at %p", CPools_GetPedRef);
        else                 loge("Failed to resolve CPools::GetPedRef!");

        if(CPools_GetPed)    logi("CPools::GetPed resolved at %p", CPools_GetPed);
        else                 loge("Failed to resolve CPools::GetPed!");

        // Hook game process
        HOOKSYM(CGame_Process, hGTAVC, "_ZN5CGame7ProcessEv");
        logi("ViceCityMod loaded! Using player + pools access.");
    }
    else
    {
        loge("Failed to load libGTAVC.so!");
    }
}
