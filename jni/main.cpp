#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>

//libR1.so
//libGTASA.so
MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

// Library handles
void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

DECL_HOOKv(SetBigMessage, unsigned short* text , unsigned short style)
{
    char16_t myString[] = u"MEGAMIND";
    SetBigMessage(reinterpret_cast<unsigned short*>(myString), 1);
}

DECL_HOOKv(SetHelpMessage, unsigned short* text, bool flag1, bool flag2, bool flag3)
{
    char16_t myString2[] = u"MEGAMIND";
    SetHelpMessage(reinterpret_cast<unsigned short*>(myString2), true, false, false);
}

// Entry point
extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (pGTAVC && hGTAVC)
    {
        HOOKSYM(SetBigMessage, hGTAVC, "_ZN4CHud13SetBigMessageEPtt");
        HOOKSYM(SetHelpMessage, hGTAVC, "_ZN4CHud14SetHelpMessageEPtbbb");
    }
}