#include "rwimage.h"

// Add STB image implementation
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_SIMD
#include "stb_image.h"

#include <android/log.h>
#include <string.h>
#include <mod/amlmod.h>

#define LOG_TAG "internetimg"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// RW Function pointers
RwRasterchatpng* (*RwRasterCreatechatpng)(int, int, int, int) = nullptr;
void* (*RwRasterLockchatpng)(RwRasterchatpng*, unsigned char, int) = nullptr;
void (*RwRasterUnlockchatpng)(RwRasterchatpng*) = nullptr;
RwTexturechatpng* (*RwTextureCreatechatpng)(RwRasterchatpng*) = nullptr;
void (*RwTextureDestroychatpng)(RwTexturechatpng*) = nullptr;

// Destructor implementation
RWImagechatpng::~RWImagechatpng() {
    Cleanupchatpng();
}

void RWImagechatpng::Cleanupchatpng() {
    if (texturechatpng && RwTextureDestroychatpng) {
        RwTextureDestroychatpng(texturechatpng);
        texturechatpng = nullptr;
    }
    if (spritechatpng) {
        delete spritechatpng;
        spritechatpng = nullptr;
    }
    datachatpng.clear();
    sizechatpng = 0;
    originalWidthchatpng = 0;
    originalHeightchatpng = 0;
    widthchatpng = 0;
    heightchatpng = 0;
}

void InitRwFunctionschatpng(void* hGTAVC, uintptr_t basechatpng)
{
    RwRasterCreatechatpng   = (decltype(RwRasterCreatechatpng))   (uintptr_t)aml->GetSym(hGTAVC, "_Z14RwRasterCreateiiii");
    RwRasterLockchatpng     = (decltype(RwRasterLockchatpng))     (uintptr_t)aml->GetSym(hGTAVC, "_Z12RwRasterLockP8RwRasterhi");
    RwRasterUnlockchatpng   = (decltype(RwRasterUnlockchatpng))   (uintptr_t)aml->GetSym(hGTAVC, "_Z14RwRasterUnlockP8RwRaster");
    RwTextureCreatechatpng  = (decltype(RwTextureCreatechatpng))  (uintptr_t)aml->GetSym(hGTAVC, "_Z15RwTextureCreateP8RwRaster");
    RwTextureDestroychatpng = (decltype(RwTextureDestroychatpng)) (uintptr_t)aml->GetSym(hGTAVC, "_Z16RwTextureDestroyP9RwTexture");
    
    LOGI("RW functions initialized: RasterCreate=%p, TextureCreate=%p", 
         RwRasterCreatechatpng, RwTextureCreatechatpng);
}

bool RWImagechatpng::LoadFromMemorychatpng(const unsigned char* data, size_t dataSize)
{
    if (!data || dataSize == 0) {
        LOGE("Invalid image data");
        return false;
    }

    // Clean up any existing texture
    Cleanupchatpng();

    // Store the data
    datachatpng.assign(data, data + dataSize);
    sizechatpng = dataSize;

    int wchatpng, hchatpng, channelschatpng;
    unsigned char* pixelschatpng = stbi_load_from_memory(
        datachatpng.data(), static_cast<int>(sizechatpng), 
        &wchatpng, &hchatpng, &channelschatpng, STBI_rgb_alpha
    );

    if (!pixelschatpng)
    {
        LOGE("stb_image failed to decode image");
        datachatpng.clear();
        sizechatpng = 0;
        return false;
    }

    originalWidthchatpng = wchatpng;
    originalHeightchatpng = hchatpng;
    widthchatpng = wchatpng * scalechatpng;
    heightchatpng = hchatpng * scalechatpng;

    if (!RwRasterCreatechatpng) {
        LOGE("RwRasterCreatechatpng is null!");
        stbi_image_free(pixelschatpng);
        return false;
    }

    RwRasterchatpng* rasterchatpng = RwRasterCreatechatpng(wchatpng, hchatpng, 32, 4);
    if (!rasterchatpng)
    {
        LOGE("RwRasterCreate failed");
        stbi_image_free(pixelschatpng);
        return false;
    }

    void* dstchatpng = RwRasterLockchatpng(rasterchatpng, 0, 1);
    if (!dstchatpng)
    {
        LOGE("RwRasterLock failed");
        stbi_image_free(pixelschatpng);
        return false;
    }

    // Flip the image vertically during copy (fixes upside-down issue)
    int stride = wchatpng * 4;
    for (int y = 0; y < hchatpng; y++) {
        // Copy each row from bottom to top
        memcpy((unsigned char*)dstchatpng + (y * stride), 
               pixelschatpng + ((hchatpng - 1 - y) * stride), 
               stride);
    }

    RwRasterUnlockchatpng(rasterchatpng);
    stbi_image_free(pixelschatpng);

    texturechatpng = RwTextureCreatechatpng(rasterchatpng);
    spritechatpng = new CSprite2dchatpng();
    spritechatpng->m_pTexturechatpng = texturechatpng;

    LOGI("Image loaded successfully: %dx%d (scaled to %fx%f)", 
         originalWidthchatpng, originalHeightchatpng, widthchatpng, heightchatpng);
    return true;
}

bool RWImagechatpng::LoadFromFilechatpng(const char* filename)
{
    LOGE("LoadFromFile not implemented - use LoadFromMemory for HTTPS");
    return false;
}

void RWImagechatpng::Drawchatpng(void* drawFnchatpng, const CRGBAchatpng& colorchatpng)
{
    if (!spritechatpng || !texturechatpng || !drawFnchatpng || !visiblechatpng) return;

    CRectchatpng chatDrawRect = { xchatpng, ychatpng, xchatpng + widthchatpng, ychatpng + heightchatpng };
    ((void(*)(CSprite2dchatpng*, const CRectchatpng&, const CRGBAchatpng&))drawFnchatpng)(spritechatpng, chatDrawRect, colorchatpng);
}