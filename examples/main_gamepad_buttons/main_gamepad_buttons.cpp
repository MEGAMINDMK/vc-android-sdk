#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <android/log.h>
#include <fstream>

#include "includes/scripting.h"

// Logging macros
#define LOG_TAG "vc_mp"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// AML Mod Info
MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

// Global Variables
void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;


// Hook: CHud::Draw
DECL_HOOKv(Draw)
{
    typedef void* (*GetPad_t)(int);
    typedef int (*Fn_t)(void*);

    static GetPad_t GetPad = (GetPad_t)aml->GetSym(hGTAVC, "_ZN4CPad6GetPadEi");

    // List of functions to test
    static Fn_t DuckJustDown     = (Fn_t)aml->GetSym(hGTAVC, "_ZN4CPad12DuckJustDownEv"); //ok
    static Fn_t HornJustDown     = (Fn_t)aml->GetSym(hGTAVC, "_ZN4CPad12HornJustDownEv"); //ok
    static Fn_t JumpJustDown     = (Fn_t)aml->GetSym(hGTAVC, "_ZN4CPad12JumpJustDownEv"); //ok
    static Fn_t WeaponJustDown   = (Fn_t)aml->GetSym(hGTAVC, "_ZN4CPad14WeaponJustDownEv"); //ok
    static Fn_t ExitVehJustDown  = (Fn_t)aml->GetSym(hGTAVC, "_ZN4CPad19ExitVehicleJustDownEv"); //ok
    static Fn_t CarGunJustDown   = (Fn_t)aml->GetSym(hGTAVC, "_ZN4CPad14CarGunJustDownEv"); //ok
    static Fn_t TargetJustDown   = (Fn_t)aml->GetSym(hGTAVC, "_ZN4CPad14TargetJustDownEv"); //ok
    static Fn_t CamModeJustDown  = (Fn_t)aml->GetSym(hGTAVC, "_ZN4CPad23CycleCameraModeJustDownEv"); //ok

    void* pad = GetPad ? GetPad(0) : nullptr;

    if (pad)
    {
        if (DuckJustDown && DuckJustDown(pad)) LOGI("DuckJustDown triggered");
        if (HornJustDown && HornJustDown(pad)) LOGI("HornJustDown triggered");
        if (JumpJustDown && JumpJustDown(pad)) LOGI("JumpJustDown triggered");
        if (WeaponJustDown && WeaponJustDown(pad)) LOGI("WeaponJustDown triggered");
        if (ExitVehJustDown && ExitVehJustDown(pad)) LOGI("ExitVehicleJustDown triggered");
        if (CarGunJustDown && CarGunJustDown(pad)) LOGI("CarGunJustDown triggered");
        if (TargetJustDown && TargetJustDown(pad)) LOGI("TargetJustDown triggered");
        if (CamModeJustDown && CamModeJustDown(pad)) LOGI("CameraModeJustDown triggered");
    }

    Draw(); // Call original
}


// Entry Point
extern "C" void OnModLoad()
{
    LOGI("OnModLoad() called!");

    pGTAVC = aml->GetLib("libGTAVC.so");
    if (!pGTAVC) {
        LOGE("Failed to find base address of libGTAVC.so!");
        return;
    }
    LOGI("Base address of libGTAVC.so: 0x%X", pGTAVC);

    hGTAVC = aml->GetLibHandle("libGTAVC.so");
    if (!hGTAVC) {
        LOGE("Failed to get handle of libGTAVC.so!");
        return;
    }
    LOGI("Successfully got libGTAVC.so handle: %p", hGTAVC);


    LOGI("Hooking CHud::Draw...");
    HOOKSYM(Draw, hGTAVC, "_ZN4CHud4DrawEv");

    // Init scripting
    LOGI("Initializing scripting system...");
    InitializeScripting();

    LOGI("Mod loaded successfully!");
} 