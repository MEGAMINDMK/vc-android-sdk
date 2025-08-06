#pragma once
#include <cstdint>
#include <cstddef>

// Add these critical forward declarations
struct RwRasterchatpng;
struct RwTexturechatpng;

struct CRGBAchatpng {
    uint8_t r, g, b, a;
};

struct CSprite2dchatpng {
    RwTexturechatpng* m_pTexturechatpng;
};

struct CRectchatpng {
    float left, top, right, bottom;
};

struct RWImagechatpng {
    const unsigned char* datachatpng;
    std::size_t sizechatpng;
    float xchatpng, ychatpng, scalechatpng;
    int widthchatpng = 0;
    int heightchatpng = 0;
    RwTexturechatpng* texturechatpng = nullptr;
    CSprite2dchatpng* spritechatpng = nullptr;

    ~RWImagechatpng();
    bool Loadchatpng();
    void Drawchatpng(void* drawFnchatpng, const CRGBAchatpng& colorchatpng);
};

// Ensure this is declared
void InitRwFunctionschatpng(void* hGTAVC, uintptr_t baseAddrchatpng);