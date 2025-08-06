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
#define LOG_TAG "ViceCityMod"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

const char* GetKeyName(int keycode)
{
    switch (keycode)
    {
        case 1045: return "ENTER";      // Found from log
        case 1042: return "BACKSPACE";
        case 32:  return "SPACE";
        case 48:  return "0";
        case 49:  return "1";
        case 50:  return "2";
        case 51:  return "3";
        case 52:  return "4";
        case 53:  return "5";
        case 54:  return "6";
        case 55:  return "7";
        case 56:  return "8";
        case 57:  return "9";
        case 65:  return "A";
        case 66:  return "B";
        case 67:  return "C";
        case 68:  return "D";
        case 69:  return "E";
        case 70:  return "F";
        case 71:  return "G";
        case 72:  return "H";
        case 73:  return "I";
        case 74:  return "J";
        case 75:  return "K";
        case 76:  return "L";
        case 77:  return "M";
        case 78:  return "N";
        case 79:  return "O";
        case 80:  return "P";
        case 81:  return "Q";
        case 82:  return "R";
        case 83:  return "S";
        case 84:  return "T";
        case 85:  return "U";
        case 86:  return "V";
        case 87:  return "W";
        case 88:  return "X";
        case 89:  return "Y";
        case 90:  return "Z";
        case 1046:return "CAPSLOCK";

        default:
            printf("Unknown keycode: %d\n", keycode); // Add this for debugging
            return "UNKNOWN";
    }
}



DECL_HOOKv(SetHelpMessage, unsigned short* text, bool flag1, bool flag2, bool flag3)
{
SetHelpMessage(text, flag1, flag2, flag3);
}
/* for 1 key at a time mostly like key press mode activate..!
DECL_HOOK(int, KeyboardHandler, int event, void* param)
{
    if (param)
    {
        int keycode = *(int*)param;
        const char* keyName = GetKeyName(keycode);
        LOGD("KeyboardEventHandler: Event: %d | Keycode: %d (%s)", event, keycode, keyName);
        size_t len = strlen(keyName);
        std::vector<char16_t> myString2(len + 1);
        mbstowcs(reinterpret_cast<wchar_t*>(myString2.data()), keyName, len + 1);
        SetHelpMessage(reinterpret_cast<unsigned short*>(myString2.data()), true, false, false);
    }
    return KeyboardHandler(event, param);
}
*/

// ================================ MEGAMINDS KEYBOARD HANDLER ======================================
std::u16string currentMessage = u"";
DECL_HOOK(int, KeyboardHandler, int event, void* param)
{
    if (param && event == 28)
    {
        int keycode = *(int*)param;
        const char* keyName = GetKeyName(keycode);
        LOGD("KeyboardEventHandler: Event: %d | Keycode: %d (%s)", event, keycode, keyName);

        if (keycode == 1042)
        {
            if (!currentMessage.empty())
            currentMessage.pop_back();
        }
      
        else if (keycode == 1045)
        {
		if (!currentMessage.empty())
        currentMessage += u"\n";
        }
        
        else if (keycode == 32)
        {
        if (!currentMessage.empty())
        currentMessage += u" ";
        }
        
        else
        {
            if (strcmp(keyName, "UNKNOWN") != 0 &&
                strcmp(keyName, "BACKSPACE") != 0 &&
                strcmp(keyName, "ENTER") != 0 &&
                strcmp(keyName, "SPACE") != 0)
            {
                size_t len = strlen(keyName);
                std::u16string myString2(len, u'\0');
                for (size_t i = 0; i < len; ++i)
                    myString2[i] = static_cast<char16_t>(keyName[i]);

                currentMessage += myString2;
            }
        }

        SetHelpMessage(reinterpret_cast<unsigned short*>(const_cast<char16_t*>(currentMessage.c_str())), true, false, false);
    }

    return KeyboardHandler(event, param);
}
// ================================ MEGAMINDS KEYBOARD HANDLER ENDS HERE ====================================
extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if (pGTAVC && hGTAVC)
    {
        HOOKSYM(KeyboardHandler, hGTAVC, "_Z15KeyboardHandler7RsEventPv");
		HOOKSYM(SetHelpMessage, hGTAVC, "_ZN4CHud14SetHelpMessageEPtbbb");
        LOGD("Keyboard handler hooked successfully!");
    }
    else
    {
        LOGE("Failed to load GTA VC library!");
    }
}
