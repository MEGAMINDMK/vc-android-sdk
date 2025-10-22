#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>

// Add these critical forward declarations
struct RwRasterchatpng;
struct RwTexturechatpng;

struct CRGBAchatpng {
    uint8_t r, g, b, a;
    CRGBAchatpng() : r(255), g(255), b(255), a(255) {}
    CRGBAchatpng(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255) 
        : r(_r), g(_g), b(_b), a(_a) {}
};

struct CSprite2dchatpng {
    RwTexturechatpng* m_pTexturechatpng;
    CSprite2dchatpng() : m_pTexturechatpng(nullptr) {}
};

struct CRectchatpng {
    float left, top, right, bottom;
    CRectchatpng() : left(0), top(0), right(0), bottom(0) {}
    CRectchatpng(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}
};

struct RWImagechatpng {
    std::vector<unsigned char> datachatpng;
    std::size_t sizechatpng;
    float xchatpng, ychatpng, widthchatpng, heightchatpng, scalechatpng;
    int originalWidthchatpng = 0;
    int originalHeightchatpng = 0;
    RwTexturechatpng* texturechatpng = nullptr;
    CSprite2dchatpng* spritechatpng = nullptr;
    bool visiblechatpng;

    RWImagechatpng() : sizechatpng(0), xchatpng(100.0f), ychatpng(100.0f), 
                      widthchatpng(0), heightchatpng(0), scalechatpng(1.0f), 
                      texturechatpng(nullptr), spritechatpng(nullptr), visiblechatpng(true) {}
    ~RWImagechatpng();
    bool LoadFromMemorychatpng(const unsigned char* data, size_t dataSize);
    bool LoadFromFilechatpng(const char* filename);
    void Drawchatpng(void* drawFnchatpng, const CRGBAchatpng& colorchatpng);
    void SetPositionchatpng(float x, float y) { xchatpng = x; ychatpng = y; }
    void SetSizechatpng(float width, float height) { widthchatpng = width; heightchatpng = height; }
    void SetScalechatpng(float scale) { 
        scalechatpng = scale; 
        if (originalWidthchatpng > 0 && originalHeightchatpng > 0) {
            widthchatpng = originalWidthchatpng * scale;
            heightchatpng = originalHeightchatpng * scale;
        }
    }
    void SetVisiblechatpng(bool visible) { visiblechatpng = visible; }
    void Cleanupchatpng();
};

// Declare the RW function pointers
extern RwRasterchatpng* (*RwRasterCreatechatpng)(int, int, int, int);
extern void* (*RwRasterLockchatpng)(RwRasterchatpng*, unsigned char, int);
extern void (*RwRasterUnlockchatpng)(RwRasterchatpng*);
extern RwTexturechatpng* (*RwTextureCreatechatpng)(RwRasterchatpng*);
extern void (*RwTextureDestroychatpng)(RwTexturechatpng*);

// Ensure this is declared
void InitRwFunctionschatpng(void* hGTAVC, uintptr_t baseAddrchatpng);