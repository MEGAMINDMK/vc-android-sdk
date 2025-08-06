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
#include <cinttypes>
#include <time.h>
#define LOG_TAG "ViceCityMod"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

// defines
void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

// Game Vars
uint8_t* ms_nGameClockHours;
uint8_t* ms_nGameClockMinutes;
uint8_t* ms_nGameClockSeconds;
int* DaysPassed;

int lastDay;

// Helpers
inline struct tm* Now()
{
    time_t tmp = time(NULL);
    return localtime(&tmp);
}

// Hook
DECL_HOOKv(ClockUpdate_VC)
{
    auto now = Now();

    if (now->tm_yday != lastDay)
        ++(*DaysPassed);

    lastDay = now->tm_yday;

    int hour12 = now->tm_hour % 12;
    if (hour12 == 0) hour12 = 12;

    *ms_nGameClockHours = static_cast<uint8_t>(hour12);
    *ms_nGameClockMinutes = now->tm_min;
	*ms_nGameClockSeconds = now->tm_sec;

}

// OnModLoad
extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (pGTAVC && hGTAVC)
    {
        HOOKSYM(ClockUpdate_VC, hGTAVC, "_ZN6CClock6UpdateEv");
        SET_TO(DaysPassed, aml->GetSym(hGTAVC, "_ZN6CStats10DaysPassedE"));
        SET_TO(ms_nGameClockHours,   aml->GetSym(hGTAVC, "_ZN6CClock18ms_nGameClockHoursE"));
        SET_TO(ms_nGameClockMinutes, aml->GetSym(hGTAVC, "_ZN6CClock20ms_nGameClockMinutesE"));
        SET_TO(ms_nGameClockSeconds, aml->GetSym(hGTAVC, "_ZN6CClock20ms_nGameClockSecondsE"));

        LOGD("GTA VC Real Time hooked successfully!");
    }
    else
    {
        LOGE("Failed to load GTA VC library!");
    }

    lastDay = Now()->tm_yday;
}