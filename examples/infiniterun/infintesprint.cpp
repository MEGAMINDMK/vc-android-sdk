#include <mod/amlmod.h>
#include <mod/logger.h>
#include <stdint.h>
#include <dlfcn.h>
#include <android/log.h>
#include <mod/config.h>
MYMODCFG(MEGAMIND.VCSPRINT, InfiniteSprintMod, 1.0, MEGAMIND)

#ifndef LOG_TAG
#define LOG_TAG "vcmp"
#endif
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

// === Hook: Prevent Sprint Energy Drain ===
DECL_HOOKv(CPlayerPed_UseSprintEnergy, void* self)
{
    // Skip original logic = infinite sprint
    LOGD("Sprint energy use intercepted — infinite sprint enabled.");
    return;
}

// === Mod Entry Point ===
extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (pGTAVC && hGTAVC)
    {
        // Hook UseSprintEnergy
        HOOKSYM(CPlayerPed_UseSprintEnergy, hGTAVC, "_ZN10CPlayerPed15UseSprintEnergyEv");

        LOGD("Infinite Sprint Mod loaded successfully.");
    }
    else
    {
        LOGE("Failed to load libGTAVC!");
    }
}
