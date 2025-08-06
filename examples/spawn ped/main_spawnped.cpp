#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>
#include <android/log.h>
#define LOG_TAG "ViceCityMod"
#define logi(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// === Mod Info ===
MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

// CVector definition
struct CVector {
    float x, y, z;

    bool operator!=(const CVector& other) const {
        return x != other.x || y != other.y || z != other.z;
    }
};

// CPed struct up to position
struct CPed {
    void* vtable;
    char pad[0x30];
    CVector position;
};

// Function typedefs
typedef CPed* (*FindPlayerPed_t)();
typedef CPed* (*AddPed_t)(int pedType, uint32_t modelId, const CVector& position, int unk);
typedef void (*RequestModel_t)(int modelId, int flags);
typedef void (*LoadAllRequestedModels_t)(bool keepInMemory);

// Function pointers
FindPlayerPed_t FindPlayerPed = nullptr;
AddPed_t AddPed = nullptr;
RequestModel_t RequestModel = nullptr;
LoadAllRequestedModels_t LoadAllRequestedModels = nullptr;

// Global state
CVector lastLoggedPos = {0.0f, 0.0f, 0.0f};
bool hasLoggedOnce = false;
int spawnDelayFrames = 3600; // ~1 minute at 60 FPS
bool hasSpawnedPed = false;

DECL_HOOKv(CGame_Process)
{
    if (FindPlayerPed) {
        CPed* ped = FindPlayerPed();
        if (ped) {
            CVector currentPos = ped->position;
            if (!hasLoggedOnce || currentPos != lastLoggedPos) {
                logi("Player Pos: X: %.2f, Y: %.2f, Z: %.2f", currentPos.x, currentPos.y, currentPos.z);
                lastLoggedPos = currentPos;
                hasLoggedOnce = true;
            }

            // Spawn ped after delay
            if (!hasSpawnedPed) {
                if (spawnDelayFrames <= 0) {
                    if (AddPed && RequestModel && LoadAllRequestedModels) {
                        int modelId = 7; // Use a stable, safe ped model

                        RequestModel(modelId, 0);
                        LoadAllRequestedModels(false);

                        CVector spawnPos = currentPos;
                        spawnPos.x += 2.0f;

                        logi("Spawning ped...");

                        CPed* newPed = AddPed(4, modelId, spawnPos, 0);
                        if (newPed) {
                            logi("Spawned ped ID %d at %.2f, %.2f, %.2f",
                                 modelId, spawnPos.x, spawnPos.y, spawnPos.z);
                            hasSpawnedPed = true;
                        } else {
                            loge("Ped spawn failed!");
                        }
                    }
                } else {
                    spawnDelayFrames--;
                }
            }
        }
    }

    CGame_Process(); // Call the original function
}

extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (pGTAVC && hGTAVC) {
        FindPlayerPed = (FindPlayerPed_t)aml->GetSym(hGTAVC, "_Z13FindPlayerPedv");
        AddPed = (AddPed_t)aml->GetSym(hGTAVC, "_ZN11CPopulation6AddPedE8ePedTypejRK7CVectori");
        RequestModel = (RequestModel_t)aml->GetSym(hGTAVC, "_ZN10CStreaming12RequestModelEii");
        LoadAllRequestedModels = (LoadAllRequestedModels_t)aml->GetSym(hGTAVC, "_ZN10CStreaming22LoadAllRequestedModelsEb");

        if (!FindPlayerPed || !AddPed || !RequestModel || !LoadAllRequestedModels) {
            loge("Critical function(s) missing!");
            return;
        }

        HOOKSYM(CGame_Process, hGTAVC, "_ZN5CGame7ProcessEv");
        logi("Mod loaded. Ped will spawn after 1 minute.");
    } else {
        loge("Failed to load libGTAVC.so!");
    }
}
