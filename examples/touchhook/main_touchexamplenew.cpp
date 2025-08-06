#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <android/log.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>
#include "includes/scripting.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define LOG_TAG "vc_mp"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;
// === Touch tracking ===
int lastTouchType = -1;
int lastTouchX = -1;
int lastTouchY = -1;
bool IsInActivationArea(int x, int y)
{
    return (x >= 0 && x <= 1000 && y >= 0 && y <= 1000);
}

DECL_HOOKv(AND_TouchEvent, int type, int x, int y, int id)
{
    lastTouchType = type;
    lastTouchX = x;
    lastTouchY = y;

   // LOGI("Touch event: type=%d, x=%d, y=%d, id=%d", type, x, y, id);

    // Trigger only on touch release within activation area
    if (type == 1 && IsInActivationArea(x, y))
    {
        LOGI("Typing mode activated in top-left area!");
      
    }

    AND_TouchEvent(type, x, y, id);
}


extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if(pGTAVC && hGTAVC)
    {
        // Hook the touch function
        HOOKSYM(AND_TouchEvent, hGTAVC, "_Z14AND_TouchEventiiii");
    }
}