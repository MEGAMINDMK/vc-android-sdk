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
struct CVector
{
    float x, y, z;

    bool operator!=(const CVector& other) const
    {
        return x != other.x || y != other.y || z != other.z;
    }
};

// CPed struct up to position
struct CPed
{
    void* vtable;
    char pad[0x30];
    CVector position;
};

// Function typedef for FindPlayerPed
typedef CPed* (*FindPlayerPed_t)();
FindPlayerPed_t FindPlayerPed = nullptr;

// Store last logged position
CVector lastLoggedPos = { 0.0f, 0.0f, 0.0f };
bool hasLoggedOnce = false;

// === Predefined teleport locations ===
CVector posBeach     = { 256.37f, -1392.12f, 11.07f };
CVector Diaz   = { -300.77, -619.32, 10.35};
// === Set player position ===
void SetPlayerPosition(const CVector& vec)
{
    if (FindPlayerPed)
    {
        CPed* ped = FindPlayerPed();
        if (ped)
        {
            ped->position = vec;
            logi("Teleported to -> X: %.2f, Y: %.2f, Z: %.2f", vec.x, vec.y, vec.z);
        }
    }
}

// === Hook into CGame::Process ===
DECL_HOOKv(CGame_Process)
{
    if (FindPlayerPed)
    {
        CPed* ped = FindPlayerPed();
        if (ped)
        {
            CVector currentPos = ped->position;

            if (!hasLoggedOnce || currentPos != lastLoggedPos)
            {
                logi("Player Pos -> X: %.2f, Y: %.2f, Z: %.2f", currentPos.x, currentPos.y, currentPos.z);
                lastLoggedPos = currentPos;
                hasLoggedOnce = true;
            }

            // === Teleport once to helipad ===
            static bool teleported = false;
            if (!teleported)
            {
                SetPlayerPosition(posBeach); // Change to posBeach or posMallRoof if you like
                teleported = true;
            }
        }
    }

    CGame_Process(); // Call original function
}

// === Mod entry point ===
extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (pGTAVC && hGTAVC)
    {
        FindPlayerPed = (FindPlayerPed_t)aml->GetSym(hGTAVC, "_Z13FindPlayerPedv");

        if (!FindPlayerPed)
        {
            loge("Failed to resolve FindPlayerPed!");
            return;
        }

        HOOKSYM(CGame_Process, hGTAVC, "_ZN5CGame7ProcessEv");
        logi("ViceCityMod loaded. Hooked CGame::Process.");
    }
    else
    {
        loge("Failed to load libGTAVC.so!");
    }
}