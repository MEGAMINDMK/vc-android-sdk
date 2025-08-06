#include <jni.h>
#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>
#include <string>
#include <android/log.h>
#define LOG_TAG "ViceCityMod"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

// defines
void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;


DECL_HOOKv(FXMaterialSetEnvMap, void* material, float coef)
{
    FXMaterialSetEnvMap(material, 4.0f);
}

DECL_HOOKv(SetEnvMapPvf, void* material, float coef)
{
	SetEnvMapPvf(material, 9.0f);
}
//============================================================================================================

extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (pGTAVC && hGTAVC)
    {
		HOOKSYM(FXMaterialSetEnvMap, hGTAVC, "_Z35RpMatFXMaterialSetEnvMapCoefficientP10RpMaterialf");
        HOOKSYM(SetEnvMapPvf, hGTAVC, "_Z13emu_SetEnvMapPvf");
	
    }
    else
    {
        LOGE("Failed to load GTA VC library!");
    }
	

	
}
