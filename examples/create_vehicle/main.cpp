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

// ---------- Forward Declared ----------
struct CVehicle;  // forward declaration only

// ---------- Typedefs ----------
typedef CVehicle* (*FindPlayerVehicleFn)(void);
typedef int       (*CPools_GetVehicleRefFn)(CVehicle*);
typedef CVehicle* (*CPools_GetVehicleFn)(int handle);

// ---------- Globals ----------
static FindPlayerVehicleFn    FindPlayerVehicle    = nullptr;
static CPools_GetVehicleRefFn CPools_GetVehicleRef = nullptr;
static CPools_GetVehicleFn    CPools_GetVehicle    = nullptr;

// ---------- Helpers ----------
bool EnsureModelLoaded(int modelId)
{
    ScriptCommand(&scm_REQUEST_MODEL, modelId);
    ScriptCommand(&scm_LOAD_ALL_MODELS_NOW);

    int available = 0, loaded = 0;
    ScriptCommand(&scm_IS_MODEL_AVAILABLE, modelId, &available);
    ScriptCommand(&scm_HAS_MODEL_LOADED, modelId, &loaded);

    return (available && loaded);
}

void SpawnCarAt(float x, float y, float z, int modelId)
{
    int carHandle = 0;
    if(!EnsureModelLoaded(modelId))
    {
        loge("Model %d not loaded yet!", modelId);
        return;
    }

    ScriptCommand(&scm_CREATE_CAR, modelId, x, y, z, &carHandle);

    if(carHandle > 0)
    {
        logi("Created vehicle (model %d) at handle %d (%.2f, %.2f, %.2f)", 
             modelId, carHandle, x, y, z);

        if(CPools_GetVehicle)
        {
            CVehicle* pCar = CPools_GetVehicle(carHandle);
            if(pCar)
            {
                logi("Vehicle pointer from handle: %p", pCar);
            }
        }
    }
    else
    {
        loge("Failed to create vehicle with model %d", modelId);
    }
}

// ========== Hook ==========
DECL_HOOKv(CGame_Process)
{
    static bool spawned = false;
    if(!spawned)
    {
        // Example: spawn some cars at Ocean View Hotel
        float x = 256.37f, y = -1392.12f, z = 11.07f;
        SpawnCarAt(x,      y, z, 166);
        SpawnCarAt(x + 5,  y, z, 191);
        SpawnCarAt(x + 10, y, z, 198);
        SpawnCarAt(x + 15, y, z, 193);

        spawned = true;
    }

    // Debug: Print current player vehicle handle
    if(FindPlayerVehicle && CPools_GetVehicleRef && CPools_GetVehicle)
    {
        CVehicle* pVeh = FindPlayerVehicle();
        if(pVeh)
        {
            int vHandle = CPools_GetVehicleRef(pVeh);
            logi("Player is in vehicle ptr=%p -> handle=%d", pVeh, vHandle);
        }
    }

    CGame_Process(); // continue normal game loop
}

// ========== Entry ==========
void* hGTAVC   = nullptr;
uintptr_t pGTAVC = 0;

extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if(pGTAVC && hGTAVC)
    {
        InitializeScripting();

        // Resolve vehicle functions
        FindPlayerVehicle    = (FindPlayerVehicleFn) aml->GetSym(hGTAVC, "_Z17FindPlayerVehiclev");
        CPools_GetVehicleRef = (CPools_GetVehicleRefFn) aml->GetSym(hGTAVC, "_ZN6CPools16GetVehicleRefEP8CVehicle");
        CPools_GetVehicle    = (CPools_GetVehicleFn) aml->GetSym(hGTAVC, "_ZN6CPools10GetVehicleEi");

        if(FindPlayerVehicle)    logi("FindPlayerVehicle resolved");
        if(CPools_GetVehicleRef) logi("CPools::GetVehicleRef resolved");
        if(CPools_GetVehicle)    logi("CPools::GetVehicle resolved");

        // Hook main game loop
        HOOKSYM(CGame_Process, hGTAVC, "_ZN5CGame7ProcessEv");
        logi("ViceCityMod loaded: vehicle spawning + pool functions ready.");
    }
    else
    {
        loge("Failed to load libGTAVC.so!");
    }
}
