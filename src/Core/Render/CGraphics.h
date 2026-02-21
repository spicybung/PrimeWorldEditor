#ifndef CGRAPHICS_H
#define CGRAPHICS_H

#include "Core/Resource/CLight.h"
#include <Common/CColor.h>
#include <Common/Math/CMatrix4f.h>
#include <Common/Math/CVector4f.h>

#include <GL/glew.h>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

class CBoneTransformData;
class CUniformBuffer;
class CVertexArrayManager;

/**
 * todo: this entire thing needs to be further abstracted, other classes shouldn't
 * need to get this close to the metal - makes it harder to extend and harder to
 * theoretically add support for other kinds of graphics backends. additionally,
 * all this stuff really shouldn't be global, and shouldn't all use public members
 * either... basically, there's a lot wrong with this system
 */
class CGraphics
{
    static std::unique_ptr<CUniformBuffer> mpMVPBlockBuffer;
    static std::unique_ptr<CUniformBuffer> mpVertexBlockBuffer;
    static std::unique_ptr<CUniformBuffer> mpPixelBlockBuffer;
    static std::unique_ptr<CUniformBuffer> mpLightBlockBuffer;
    static std::unique_ptr<CUniformBuffer> mpBoneTransformBuffer;
    static uint32_t mContextIndices;
    static uint32_t mActiveContext;
    static bool mInitialized;
    static std::vector<std::unique_ptr<CVertexArrayManager>> mVAMs;
    static bool mIdentityBoneTransforms;

public:
    // SMVPBlock
    struct SMVPBlock
    {
        CMatrix4f ModelMatrix;
        CMatrix4f ViewMatrix;
        CMatrix4f ProjectionMatrix;
    };
    static constinit SMVPBlock sMVPBlock;

    // SVertexBlock
    struct SVertexBlock
    {
        std::array<CMatrix4f, 10> TexMatrices;
        std::array<CMatrix4f, 20> PostMatrices;
        CColor COLOR0_Amb;
        CColor COLOR0_Mat;
        CColor COLOR1_Amb;
        CVector4f COLOR1_Mat;
    };
    static constinit SVertexBlock sVertexBlock;

    // SPixelBlock
    struct SPixelBlock
    {
        std::array<CColor, 4> Konst;
        std::array<CColor, 4> TevColor;
        CColor TintColor;
        float LightmapMultiplier{};
        std::array<float, 3> Padding{};

        void SetAllTevColors(const CColor& color)
        {
            TevColor.fill(color);
        }
    };
    static constinit SPixelBlock sPixelBlock;

    // SLightBlock
    struct SLightBlock
    {
        struct SGXLight
        {
            CVector4f Position;
            CVector4f Direction;
            CColor Color;
            CVector4f DistAtten;
            CVector4f AngleAtten;
        };
        std::array<SGXLight, 8> Lights;
    };
    static constinit SLightBlock sLightBlock;

    // Lighting-related
    enum class ELightingMode { None, Basic, World };
    static ELightingMode sLightMode;
    static uint32_t sNumLights;
    static constexpr CColor skDefaultAmbientColor{0.5f, 0.5f, 0.5f, 0.0f};
    static CColor sAreaAmbientColor;
    static float sWorldLightMultiplier;
    static std::array<CLight, 3> sDefaultDirectionalLights;

    // Functions
    static void Initialize();
    static void Shutdown();
    static void UpdateMVPBlock();
    static void UpdateVertexBlock();
    static void UpdatePixelBlock();
    static void UpdateLightBlock();
    static GLuint MVPBlockBindingPoint();
    static GLuint VertexBlockBindingPoint();
    static GLuint PixelBlockBindingPoint();
    static GLuint LightBlockBindingPoint();
    static GLuint BoneTransformBlockBindingPoint();
    static uint32_t GetContextIndex();
    static uint32_t GetActiveContext();
    static void ReleaseContext(uint32_t Index);
    static void SetActiveContext(uint32_t Index);
    static void SetDefaultLighting();
    static void SetupAmbientColor();
    static void SetIdentityMVP();
    static void LoadBoneTransforms(const CBoneTransformData& rkData);
    static void LoadIdentityBoneTransforms();
};

#endif // CGRAPHICS_H
