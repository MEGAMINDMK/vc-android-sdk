#include "rwimage.h"
#include "stb_image.h"
#include <android/log.h>
#include <string.h>
#include <mod/amlmod.h>

#define LOG_TAG "vc_mp"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// RW Function pointers
RwRasterchatpng* (*RwRasterCreatechatpng)(int, int, int, int) = nullptr;
void* (*RwRasterLockchatpng)(RwRasterchatpng*, unsigned char, int) = nullptr;
void (*RwRasterUnlockchatpng)(RwRasterchatpng*) = nullptr;
RwTexturechatpng* (*RwTextureCreatechatpng)(RwRasterchatpng*) = nullptr;
void (*RwTextureDestroychatpng)(RwTexturechatpng*) = nullptr;



// Destructor implementation with full type visibility
RWImagechatpng::~RWImagechatpng() {
    if (texturechatpng && RwTextureDestroychatpng) {
        RwTextureDestroychatpng(texturechatpng);
    }
    if (spritechatpng) {
        delete spritechatpng;  // Safe now that CSprite2dchatpng is fully defined
    }
}

void InitRwFunctionschatpng(void* hGTAVC, uintptr_t basechatpng)
{
    RwRasterCreatechatpng   = (decltype(RwRasterCreatechatpng))   (uintptr_t)aml->GetSym(hGTAVC, "_Z14RwRasterCreateiiii");
    RwRasterLockchatpng     = (decltype(RwRasterLockchatpng))     (uintptr_t)aml->GetSym(hGTAVC, "_Z12RwRasterLockP8RwRasterhi");
    RwRasterUnlockchatpng   = (decltype(RwRasterUnlockchatpng))   (uintptr_t)aml->GetSym(hGTAVC, "_Z14RwRasterUnlockP8RwRaster");
    RwTextureCreatechatpng  = (decltype(RwTextureCreatechatpng))  (uintptr_t)aml->GetSym(hGTAVC, "_Z15RwTextureCreateP8RwRaster");
    RwTextureDestroychatpng = (decltype(RwTextureDestroychatpng)) (uintptr_t)aml->GetSym(hGTAVC, "_Z16RwTextureDestroyP9RwTexture");
}

bool RWImagechatpng::Loadchatpng()
{
    int wchatpng, hchatpng, channelschatpng;
    unsigned char* pixelschatpng = stbi_load_from_memory(
        datachatpng, static_cast<int>(sizechatpng), &wchatpng, &hchatpng, &channelschatpng, STBI_rgb_alpha
    );

    if (!pixelschatpng)
    {
        LOGE("stb_image failed to decode chat image");
        return false;
    }

    widthchatpng = wchatpng;
    heightchatpng = hchatpng;

    RwRasterchatpng* rasterchatpng = RwRasterCreatechatpng(wchatpng, hchatpng, 32, 4);
    if (!rasterchatpng)
    {
        LOGE("RwRasterCreate failed for chat background");
        stbi_image_free(pixelschatpng);
        return false;
    }

    void* dstchatpng = RwRasterLockchatpng(rasterchatpng, 0, 1);
    if (!dstchatpng)
    {
        LOGE("RwRasterLock failed for chat background");
        stbi_image_free(pixelschatpng);
        return false;
    }

    memcpy(dstchatpng, pixelschatpng, wchatpng * hchatpng * 4);
    RwRasterUnlockchatpng(rasterchatpng);
    stbi_image_free(pixelschatpng);

    texturechatpng = RwTextureCreatechatpng(rasterchatpng);
    spritechatpng = new CSprite2dchatpng();
    spritechatpng->m_pTexturechatpng = texturechatpng;
    return true;
}

void RWImagechatpng::Drawchatpng(void* drawFnchatpng, const CRGBAchatpng& colorchatpng)
{
    if (!spritechatpng || !texturechatpng) return;

    float chatDrawWidth = widthchatpng * scalechatpng;
    float chatDrawHeight = heightchatpng * scalechatpng;
    CRectchatpng chatDrawRect = { xchatpng, ychatpng, xchatpng + chatDrawWidth, ychatpng + chatDrawHeight };

    ((void(*)(CSprite2dchatpng*, const CRectchatpng&, const CRGBAchatpng&))drawFnchatpng)(spritechatpng, chatDrawRect, colorchatpng);
}