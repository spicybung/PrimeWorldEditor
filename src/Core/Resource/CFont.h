#ifndef CFONT_H
#define CFONT_H

#include "Core/OpenGL/CDynamicVertexBuffer.h"
#include "Core/OpenGL/CIndexBuffer.h"
#include "Core/Resource/CResource.h"
#include "Core/Resource/CTexture.h"
#include "Core/Resource/TResPtr.h"
#include "Core/Resource/Model/CVertex.h"

#include <array>
#include <optional>
#include <unordered_map>
#include <vector>

class CRenderer;

class CFont : public CResource
{
    DECLARE_RESOURCE_TYPE(Font)

    static constexpr uint32_t s_font_default_size = UINT32_MAX;

    friend class CFontLoader;
    static std::optional<CDynamicVertexBuffer> smGlyphVertices; // This is the vertex buffer used to draw glyphs. It has two attributes - Pos and Tex0. Tex0 should be updated for each glyph.
    static CIndexBuffer smGlyphIndices; // This is the index buffer used to draw glyphs. It uses a triangle strip.
    static bool smBuffersInitialized;   // This bool indicates whether the vertex/index buffer have been initialized. Checked at the start of RenderString().

    uint32_t mUnknown = 0;              // Value at offset 0x8. Not sure what this is. Including for experimentation purposes.
    uint32_t mLineHeight = 0;           // Height of each line, in points
    uint32_t mLineMargin = 0;           // Gap between lines, in points - this is added to the line height
    uint32_t mVerticalOffset = 0;       // In points. This is used to reposition glyphs after the per-glyph vertical offset is applied
    uint32_t mDefaultSize = 0;          // In points.
    TString mFontName;                  // Self-explanatory
    TResPtr<CTexture> mpFontTexture;    // The texture used by this font
    uint32_t mTextureFormat = 0;        // Indicates which layers on the texture are for what - multiple glyph layers or fill/stroke

    struct SGlyph
    {
        uint16_t Character{};               // The UTF-16 character that this glyph corresponds to
        std::array<CVector2f, 4> TexCoords; // The format only lists the min/max X/Y values; tracking absolute coordinates in memory is faster
        int32_t LeftPadding{};              // The amount of padding applied left of this glyph, in points
        int32_t RightPadding{};             // The amount of padding applied right of this glyph, in points
        uint32_t Width{};                   // The width of the glyph, in points
        uint32_t Height{};                  // The height of the glyph, in points
        uint32_t PrintAdvance{};            // How far the print head advances horizontally after printing this glyph, in points
        uint32_t BaseOffset{};              // Vertical offset for this glyph, in points; the font-wide offset is added to this
        uint32_t KerningIndex{};            // Index into the kerning table of the first kerning pair for this glyph. -1 if no pairs.
        uint8_t RGBAChannel{};              // Fonts can store multiple glyphs in the same space on different RGBA channels. This value corresponds to R, G, B, or A.
    };
    std::unordered_map<uint16_t, SGlyph> mGlyphs;

    struct SKerningPair
    {
        uint16_t CharacterA{}; // Left character
        uint16_t CharacterB{}; // Right character
        int32_t Adjust{};      // The horizontal offset to apply to CharacterB if this pair is encountered, in points
    };
    std::vector<SKerningPair> mKerningTable; // The kerning table should be laid out in alphabetical order for the indices to work properly


public:
    explicit CFont(CResourceEntry *pEntry = nullptr);
    ~CFont() override;
    std::unique_ptr<CDependencyTree> BuildDependencyTree() override;
    CVector2f RenderString(const TString& rkString, CRenderer *pRenderer, float AspectRatio,
                           CVector2f Position = CVector2f(0,0),
                           CColor FillColor = CColor::White(), CColor StrokeColor = CColor::Black(),
                           uint32_t FontSize = s_font_default_size);

    // Accessors
    const TString& FontName() const { return mFontName; }
    CTexture* Texture() const       { return mpFontTexture; }
private:
    static void InitBuffers();
    static void ShutdownBuffers();
};

#endif // CFONT_H
