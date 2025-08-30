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
#define LOG_TAG "vc"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// AML Mod Info
MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

// Global Variables
void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

void SpawnCar()
{
    int carModel = 166;
	int carModels = 191;
	int carModelf = 198;
	int carModelx = 193;
    float x = 256.37f, y = -1392.12f, z = 11.07f; // ocean view hotel
    int carHandle = 0;

    // 1. Request car model
    CALLSCM(REQUEST_MODEL, carModel);
	CALLSCM(REQUEST_MODEL, carModels);
	CALLSCM(REQUEST_MODEL, carModelf);
	CALLSCM(REQUEST_MODEL, carModelx);

    // 2. Load models
    CALLSCM(LOAD_ALL_MODELS_NOW, 0); // param is dummy (z = 'zero')

    // 3. Create the car
CALLSCM(CREATE_CAR, carModel, x, y, z, &carHandle);
ScriptCommand(&scm_CREATE_CAR, carModels, x, y, z, &carHandle);

if(ScriptCommand(&scm_CREATE_CAR, carModelf, x, y, z, &carHandle))
{
    LOGI("CREATE_CAR succeeded! Handle: %d", carHandle);
}
else
{
    LOGE("CREATE_CAR failed!");
}

if(CALLSCM(CREATE_CAR, carModelx, x, y, z, &carHandle))
{
    LOGI("Car created at handle: %d", carHandle);
}else
{
    LOGE("CREATE_CAR failed!");
}
   
}

// Hook: CHud::SetHelpMessage
DECL_HOOKv(SetHelpMessage, unsigned short* text, bool flag1, bool flag2, bool flag3)
{
    char16_t myString2[] = u"MEGAMIND";
    LOGI("SetHelpMessage hook called. Showing custom message.");
    SetHelpMessage(reinterpret_cast<unsigned short*>(myString2), true, false, false);

    // SCM scripting opcodes only
    LOGI("Calling SCM opcode: FORCE_WEATHER_NOW");
    CALLSCM(FORCE_WEATHER_NOW, 5);  // 4 = Cloudy

    LOGI("Calling SCM opcode: SET_TIME_OF_DAY");
    CALLSCM(SET_TIME_OF_DAY, 0, 00); // 12:00 PM
	
	
	SpawnCar();
	


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

    // Hook CHud::SetHelpMessage
    LOGI("Hooking CHud::SetHelpMessage...");
    HOOKSYM(SetHelpMessage, hGTAVC, "_ZN4CHud14SetHelpMessageEPtbbb");

    // Initialize scripting
    LOGI("Initializing scripting system...");
    InitializeScripting();

    LOGI("Mod loaded successfully!");
}
