#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <android/log.h>
#include <stdint.h>
#include <unistd.h>  // for usleep
#include "includes/scripting.h"

#define LOG_TAG "ViceCityMod"
#define logi(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ========== Mod Info ==========
MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

// ========== State ==========
static bool modelLoadedLogged = false;
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

// ---------- Helper ==========
bool EnsureModelLoaded(int modelId)
{
    static bool requested = false;
    if(!requested)
    {
        ScriptCommand(&scm_REQUEST_MODEL, modelId);
        ScriptCommand(&scm_LOAD_ALL_MODELS_NOW);
        logi("Model %d requested.", modelId);
        requested = true;
    }

    int available = 0, loaded = 0;
    ScriptCommand(&scm_IS_MODEL_AVAILABLE, modelId, &available);
    ScriptCommand(&scm_HAS_MODEL_LOADED, modelId, &loaded);

    if(available && loaded && !modelLoadedLogged)
    {
        logi("Model %d is now available and loaded!", modelId);
        modelLoadedLogged = true;
    }

    return available && loaded;
}

// ========== Hook ==========
DECL_HOOKv(CGame_Process)
{
    // Always ensure model 17 is loaded
    EnsureModelLoaded(17);

    // One-time setup for player handle
    if(!hasSetup)
    {
        CPed* pPlayer = FindPlayerPed ? FindPlayerPed() : nullptr;
        if(!pPlayer)
        {
            usleep(1000); // wait and retry next frame
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
                    // Remove all weapons first
                    ScriptCommand(&scm_REMOVE_ALL_CHAR_WEAPONS, handle);

                    // ✅ Give weapon using model 17
                    ScriptCommand(&scm_GIVE_WEAPON_TO_CHAR, handle, 17, 250);
                    ScriptCommand(&scm_SET_CURRENT_CHAR_WEAPON, handle, 17);
                    logi("Player armed with weapon model 17 + 250 ammo");

                    // Dump slots for debug
                    for(int slot = 0; slot <= 12; slot++)
                    {
                        int weaponType = 0, weaponAmmo = 0, weaponModel = -1;
                        ScriptCommand(&scm_GET_CHAR_WEAPON_IN_SLOT, handle, slot, &weaponType, &weaponAmmo, &weaponModel);

                        if(weaponType != 0)
                        {
                            logi("Slot %d -> WeaponType=%d, Ammo=%d, Model=%d",
                                slot, weaponType, weaponAmmo, weaponModel);
                        }
                    }
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

        // Resolve pools + player functions
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
        logi("ViceCityMod loaded! Model loader + player handle + weapon slots ready.");
    }
    else
    {
        loge("Failed to load libGTAVC.so!");
    }
}
