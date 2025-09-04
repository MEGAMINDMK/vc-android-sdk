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

// ---------- Structs ----------
struct CVector { float x, y, z; };

// ---------- Enums ----------
enum BlipDisplay {
    Neither = 0,
    MarkerOnly = 1,
    BlipOnly = 2,
    Both = 3
};

enum BlipColor {
    Red = 0,
    Green = 1,
    LightBlue = 2,
    Gray = 3,
    Yellow = 4,
    Magenta = 5,
    Cyan = 6
};

// ---------- Typedefs ----------
typedef int  (*CRadar_SetCoordBlipFn)(int, CVector, unsigned int, int);
typedef void (*CRadar_ClearBlipFn)(int);
typedef void (*CRadar_ShowRadarMarkerFn)(CVector, unsigned int, float);

// ---------- Globals ----------
static CRadar_SetCoordBlipFn    SetCoordBlip     = nullptr;
static CRadar_ClearBlipFn       ClearBlip       = nullptr;
static CRadar_ShowRadarMarkerFn ShowRadarMarker = nullptr;

// ---------- Blip handler ----------
class Blip {
private:
    int blipId = -1;
    CVector pos;

public:
    Blip(float x, float y, float z, BlipColor color = Red, BlipDisplay display = Both) {
        pos = {x, y, z};
        if(SetCoordBlip)
            blipId = SetCoordBlip(4 /* BLIP_COORD */, pos, static_cast<unsigned int>(color), static_cast<int>(display));
        if(ShowRadarMarker)
            ShowRadarMarker(pos, static_cast<unsigned int>(color), 1.0f);
        logi("Blip created id=%d at (%.2f, %.2f, %.2f)", blipId, x, y, z);
    }

    ~Blip() { Remove(); }

    void Remove() {
        if(blipId > 0 && ClearBlip) {
            ClearBlip(blipId);
            logi("Blip removed id=%d", blipId);
            blipId = -1;
        }
    }

    void ChangeColor(BlipColor color) {
        if(blipId > 0 && ShowRadarMarker)
            ShowRadarMarker(pos, static_cast<unsigned int>(color), 1.0f);
    }

    void ChangeDisplay(BlipDisplay display) {
        if(blipId > 0 && SetCoordBlip)
            blipId = SetCoordBlip(4, pos, 0, static_cast<int>(display));
    }

    void ChangeScale(float size) {
        if(blipId > 0 && ShowRadarMarker)
            ShowRadarMarker(pos, 0, size);
    }

    int GetHandle() const { return blipId; }
};

// ---------- Example hook ----------
DECL_HOOKv(CGame_Process)
{
    static bool addedBlips = false;
    if(!addedBlips)
    {
        // Add some example blips at ocean view hotel
        Blip* b1 = new Blip(256.37f, -1392.12f, 11.07f, Red, Both);
        Blip* b2 = new Blip(261.37f, -1392.12f, 11.07f, Green, Both);
        Blip* b3 = new Blip(266.37f, -1392.12f, 11.07f, LightBlue, Both);

        logi("Example blips added: %d, %d, %d", b1->GetHandle(), b2->GetHandle(), b3->GetHandle());
        addedBlips = true;
    }

    CGame_Process(); // continue normal game loop
}

// ---------- Entry ----------
void* hGTAVC   = nullptr;
uintptr_t pGTAVC = 0;

extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if(pGTAVC && hGTAVC)
    {
        InitializeScripting();

        SetCoordBlip     = (CRadar_SetCoordBlipFn)     aml->GetSym(hGTAVC, "_ZN6CRadar12SetCoordBlipE9eBlipType7CVectorj12eBlipDisplay");
        ClearBlip        = (CRadar_ClearBlipFn)        aml->GetSym(hGTAVC, "_ZN6CRadar9ClearBlipEi");
        ShowRadarMarker  = (CRadar_ShowRadarMarkerFn)  aml->GetSym(hGTAVC, "_ZN6CRadar15ShowRadarMarkerE7CVectorjf");

        if(SetCoordBlip)     logi("CRadar::SetCoordBlip resolved");
        if(ClearBlip)        logi("CRadar::ClearBlip resolved");
        if(ShowRadarMarker)  logi("CRadar::ShowRadarMarker resolved");

        HOOKSYM(CGame_Process, hGTAVC, "_ZN5CGame7ProcessEv");
        logi("ViceCityMod loaded: radar blip handler ready.");
    }
    else
    {
        loge("Failed to load libGTAVC.so!");
    }
}
