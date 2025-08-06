#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>

MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

// -- VehicleCheat typedef and pointer --
typedef void* (*VehicleCheat_t)(int modelId);
VehicleCheat_t VehicleCheat = nullptr;

// Flag to ensure we only spawn once
bool vehicleSpawned = false;


// -- Hook: CGame::Process (Main game loop) --
DECL_HOOKv(CGame_Process)
{
    if (!vehicleSpawned && VehicleCheat)
    {
        VehicleCheat(141); // INFERNUS
        vehicleSpawned = true;
    }

    CGame_Process(); // Call original
}

// -- AML Entry Point --
extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (!pGTAVC || !hGTAVC) return;

    // Get symbol for VehicleCheat
    VehicleCheat = (VehicleCheat_t)aml->GetSym(hGTAVC, "_Z12VehicleCheati");
    // Hook CGame::Process (main game tick)
    HOOKSYM(CGame_Process, hGTAVC, "_ZN5CGame7ProcessEv");
}
