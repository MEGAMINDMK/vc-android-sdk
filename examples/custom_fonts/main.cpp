#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>
#include <fstream>
#include <vector>
#include <android/log.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define LOG_TAG "VC_SYSTEM_FONT"
#define logi(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Mod config
MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)

// --- Global vars ---
void* hGTAVC = nullptr;
uintptr_t pGTAVC = 0;

stbtt_fontinfo g_font;
std::vector<unsigned char> g_fontBuffer;
bool g_fontLoaded = false;

// GTA structs
struct alignas(4) CRGBA
{
    uint8_t r,g,b,a;
    CRGBA(uint8_t _r,uint8_t _g,uint8_t _b,uint8_t _a=255):r(_r),g(_g),b(_b),a(_a){}
};

struct CRect
{
    float left, top, right, bottom;
    CRect(float l,float t,float r,float b):left(l),top(t),right(r),bottom(b){}
};

// CSprite2d::DrawRect
typedef void (*CSprite2d_DrawRectFn)(CRect const&, CRGBA const&);
static CSprite2d_DrawRectFn CSprite2d_DrawRect = nullptr;

// --- Load any font ---
bool LoadTTFFont(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    if(!file.is_open())
    {
        loge("Could not open font: %s", path);
        return false;
    }

    g_fontBuffer.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if(!stbtt_InitFont(&g_font, g_fontBuffer.data(), stbtt_GetFontOffsetForIndex(g_fontBuffer.data(), 0)))
    {
        loge("Failed to init font from %s", path);
        g_fontBuffer.clear();
        return false;
    }

    g_fontLoaded = true;
    logi("Loaded font: %s", path);
    return true;
}

// --- Draw one character ---
void DrawGlyph(char ch, float& penX, float penY, float scale, 
               const CRGBA& color, 
               const CRGBA* shadowColor = nullptr, float shadowOffset = 0.0f)
{
    int w,h,xoff,yoff;
    unsigned char* bmp = stbtt_GetCodepointBitmap(&g_font, 0, scale, ch, &w,&h,&xoff,&yoff);

    if(bmp && CSprite2d_DrawRect)
    {
        for(int y=0;y<h;y++)
        {
            for(int x=0;x<w;x++)
            {
                unsigned char c = bmp[y*w + x];
                if(c > 0)
                {
                    float px = penX + x + xoff;
                    float py = penY + y + yoff;

                    // --- Draw shadow first if requested ---
                    if(shadowColor && shadowOffset > 0.0f)
                    {
                        CRect shadowRect(px + shadowOffset, py + shadowOffset,
                                         px + shadowOffset + 1, py + shadowOffset + 1);
                        CRGBA scol(shadowColor->r, shadowColor->g, shadowColor->b, c);
                        CSprite2d_DrawRect(shadowRect, scol);
                    }

                    // --- Draw main glyph ---
                    CRect rect(px, py, px+1, py+1);
                    CRGBA col(color.r, color.g, color.b, c);
                    CSprite2d_DrawRect(rect, col);
                }
            }
        }
        stbtt_FreeBitmap(bmp,nullptr);
    }

    // advance pen
    int advance, lsb;
    stbtt_GetCodepointHMetrics(&g_font, ch, &advance, &lsb);
    penX += advance * scale;
}

// --- Draw string ---
void DrawString(const char* text, float startX, float startY, float pixelHeight, 
                const CRGBA& color, 
                const CRGBA* shadowColor = nullptr, float shadowOffset = 0.0f)
{
    float scale = stbtt_ScaleForPixelHeight(&g_font, pixelHeight);
    float penX = startX;
    float penY = startY;

    for(const char* p=text; *p; ++p)
    {
        DrawGlyph(*p, penX, penY, scale, color, shadowColor, shadowOffset);
    }
}

// --- Read or create font.txt ---
std::string GetFontPath()
{
    const char* txtPath = "/storage/emulated/0/VCMP/font.txt";

    {
        std::ifstream check(txtPath);
        if(!check.is_open())
        {
            std::ofstream create(txtPath);
            if(create.is_open())
            {
                create << "DroidSans";
                create.close();
                logi("Created default font.txt with DroidSans");
            }
        }
    }

    std::ifstream f(txtPath);
    if(!f.is_open())
    {
        loge("Could not open font.txt, using fallback font!");
        return "/system/fonts/DroidSans.ttf";
    }

    std::string fontName;
    std::getline(f, fontName);
    f.close();

    if(fontName.empty())
    {
        loge("font.txt empty, using fallback font!");
        return "/system/fonts/DroidSans.ttf";
    }

    while(!fontName.empty() && (fontName.back()=='\r' || fontName.back()==' ')) fontName.pop_back();

    if(fontName.find('/') != std::string::npos)
    {
        logi("Using direct font path: %s", fontName.c_str());
        return fontName;
    }

    if(fontName.find('.') == std::string::npos)
    {
        fontName += ".ttf";
    }

    std::string vcmpPath = std::string("/storage/emulated/0/VCMP/") + fontName;
    std::ifstream test1(vcmpPath);
    if(test1.is_open())
    {
        test1.close();
        logi("Using VCMP font: %s", vcmpPath.c_str());
        return vcmpPath;
    }

    std::string sysPath = std::string("/system/fonts/") + fontName;
    std::ifstream test2(sysPath);
    if(test2.is_open())
    {
        test2.close();
        logi("Using system font: %s", sysPath.c_str());
        return sysPath;
    }

    loge("Font %s not found in VCMP/ or system/fonts, using fallback!", fontName.c_str());
    return "/system/fonts/DroidSans.ttf";
}

// --- Hook HUD::Draw ---
DECL_HOOKv(CHud__Draw)
{
    CHud__Draw();

    if(g_fontLoaded && CSprite2d_DrawRect)
    {
        // Example 1: green text with black shadow (2px offset)
        CRGBA mainColor(0,255,0,255);
        CRGBA shadowCol(0,0,0,255);
        DrawString("MEGAMIND with shadow!", 180.0f, 100.0f, 24.0f, mainColor, &shadowCol, 2.0f);

        // Example 2: red text with blue shadow (3px offset)
        CRGBA red(255,0,0,255);
        CRGBA blue(0,0,255,255);
        DrawString("COLOR TEST", 200.0f, 140.0f, 28.0f, red, &blue, 3.0f);

        // Example 3: yellow text with no shadow
        DrawString("NO SHADOW", 220.0f, 180.0f, 20.0f, CRGBA(255,255,0,255));
    }
}

// --- Entry ---
extern "C" void OnModLoad()
{
    pGTAVC = aml->GetLib("libGTAVC.so");
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    if(pGTAVC && hGTAVC)
    {
        HOOKSYM(CHud__Draw, hGTAVC, "_ZN4CHud4DrawEv");
        CSprite2d_DrawRect = (CSprite2d_DrawRectFn)aml->GetSym(hGTAVC, "_ZN9CSprite2d8DrawRectERK5CRectRK5CRGBA");

        if(CSprite2d_DrawRect) logi("CSprite2d::DrawRect resolved");
        else loge("Failed to resolve CSprite2d::DrawRect");
    }

    std::string fontPath = GetFontPath();
    LoadTTFFont(fontPath.c_str());
}
