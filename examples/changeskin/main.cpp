#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <android/log.h>
#include <stdint.h>

#define LOG_TAG "VC_SkinChanger"
#define logi(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

MYMODCFG(MEGAMIND.VCSkinChanger, VC_SkinChanger, 1.0, MEGAMIND)

// ========== Structs ==========
struct CVector { float x, y, z; };
struct CPed {
    void* vtable;
    uint8_t pad[0x30];
    CVector position;
};

// ========== Typedefs ==========
typedef CPed* (*FindPlayerPedFn)();
typedef void (*RequestModel_t)(int modelId, int flags);
typedef void (*LoadAllRequestedModels_t)(bool);
typedef void (*CPed_SetModelIndexFn)(CPed*, unsigned int);

// ========== Function Pointers ==========
FindPlayerPedFn FindPlayerPed;
RequestModel_t RequestModel;
LoadAllRequestedModels_t LoadAllRequestedModels;
CPed_SetModelIndexFn CPed_SetModelIndex;

// ========== State ==========
bool hasChangedModel = false;

// ========== Change Function ==========
void ChangePlayerModel(CPed* player, int modelId)
{
    if (!player || !CPed_SetModelIndex) return;

    RequestModel(modelId, 0);
    LoadAllRequestedModels(false);

    CPed_SetModelIndex(player, modelId);
    logi("Changed player model to ID: %d", modelId);
}

// ========== Hook ==========
DECL_HOOKv(CGame_Process)
{
    CPed* player = FindPlayerPed();
    if (player && !hasChangedModel)
    {
        ChangePlayerModel(player, 4); // Change model ID here
        hasChangedModel = true;
    }

    CGame_Process();
}

// ========== Entry Point ==========
void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");
    if (!pGTAVC || !hGTAVC) return;

    logi("VC_SkinChanger loaded!");

    FindPlayerPed = (FindPlayerPedFn)aml->GetSym(hGTAVC, "_Z13FindPlayerPedv");
    RequestModel = (RequestModel_t)aml->GetSym(hGTAVC, "_ZN10CStreaming12RequestModelEii");
    LoadAllRequestedModels = (LoadAllRequestedModels_t)aml->GetSym(hGTAVC, "_ZN10CStreaming22LoadAllRequestedModelsEb");
    CPed_SetModelIndex = (CPed_SetModelIndexFn)aml->GetSym(hGTAVC, "_ZN4CPed13SetModelIndexEj");

    HOOKSYM(CGame_Process, hGTAVC, "_ZN5CGame7ProcessEv");
}
