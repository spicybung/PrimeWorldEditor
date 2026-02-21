#include "Core/Render/CGraphics.h"

#include "Core/OpenGL/CShader.h"
#include "Core/OpenGL/CUniformBuffer.h"
#include "Core/OpenGL/CVertexArrayManager.h"
#include "Core/Render/CBoneTransformData.h"
#include "Core/Resource/CMaterial.h"

#include <Common/Log.h>
#include <Common/Math/CVector3f.h>
#include <Common/Math/CTransform4f.h>

// ************ MEMBER INITIALIZATION ************
std::unique_ptr<CUniformBuffer> CGraphics::mpMVPBlockBuffer;
std::unique_ptr<CUniformBuffer> CGraphics::mpVertexBlockBuffer;
std::unique_ptr<CUniformBuffer> CGraphics::mpPixelBlockBuffer;
std::unique_ptr<CUniformBuffer> CGraphics::mpLightBlockBuffer;
std::unique_ptr<CUniformBuffer> CGraphics::mpBoneTransformBuffer;
uint32_t CGraphics::mContextIndices = 0;
uint32_t CGraphics::mActiveContext = -1;
bool CGraphics::mInitialized = false;
std::vector<std::unique_ptr<CVertexArrayManager>> CGraphics::mVAMs;
bool CGraphics::mIdentityBoneTransforms = false;

constinit CGraphics::SMVPBlock    CGraphics::sMVPBlock;
constinit CGraphics::SVertexBlock CGraphics::sVertexBlock;
constinit CGraphics::SPixelBlock  CGraphics::sPixelBlock;
constinit CGraphics::SLightBlock  CGraphics::sLightBlock;

CGraphics::ELightingMode CGraphics::sLightMode = CGraphics::ELightingMode::World;
uint32_t CGraphics::sNumLights = 0;
CColor CGraphics::sAreaAmbientColor = CColor::TransparentBlack();
float CGraphics::sWorldLightMultiplier = 1.0f;
std::array<CLight, 3> CGraphics::sDefaultDirectionalLights{{
    CLight::BuildDirectional(CVector3f(0), CVector3f(0.f, -0.866025f, -0.5f), CColor(0.3f, 0.3f, 0.3f, 0.3f)),
    CLight::BuildDirectional(CVector3f(0), CVector3f(-0.75f, 0.433013f, -0.5f), CColor(0.3f, 0.3f, 0.3f, 0.3f)),
    CLight::BuildDirectional(CVector3f(0), CVector3f(0.75f, 0.433013f, -0.5f), CColor(0.3f, 0.3f, 0.3f, 0.3f)),
}};

constexpr size_t sNumBoneTransforms = 100;

// ************ FUNCTIONS ************
void CGraphics::Initialize()
{
    if (!mInitialized)
    {
        NLog::Debug("Initializing GLEW");
        glewExperimental = true;
        glewInit();
        glGetError(); // This is to work around a glew bug - error is always set after initializing

        NLog::Debug("Creating uniform buffers");
        mpMVPBlockBuffer = std::make_unique<CUniformBuffer>(sizeof(sMVPBlock));
        mpVertexBlockBuffer = std::make_unique<CUniformBuffer>(sizeof(sVertexBlock));
        mpPixelBlockBuffer = std::make_unique<CUniformBuffer>(sizeof(sPixelBlock));
        mpLightBlockBuffer = std::make_unique<CUniformBuffer>(sizeof(sLightBlock));
        mpBoneTransformBuffer = std::make_unique<CUniformBuffer>(sizeof(CTransform4f) * sNumBoneTransforms);

        mInitialized = true;
    }
    mpMVPBlockBuffer->BindBase(0);
    mpVertexBlockBuffer->BindBase(1);
    mpPixelBlockBuffer->BindBase(2);
    mpLightBlockBuffer->BindBase(3);
    mpBoneTransformBuffer->BindBase(4);
    LoadIdentityBoneTransforms();
}

void CGraphics::Shutdown()
{
    if (!mInitialized)
        return;

    NLog::Debug("Shutting down CGraphics");
    mpMVPBlockBuffer.reset();
    mpVertexBlockBuffer.reset();
    mpPixelBlockBuffer.reset();
    mpLightBlockBuffer.reset();
    mpBoneTransformBuffer.reset();
    mInitialized = false;
}

void CGraphics::UpdateMVPBlock()
{
    mpMVPBlockBuffer->Buffer(&sMVPBlock);
}

void CGraphics::UpdateVertexBlock()
{
    mpVertexBlockBuffer->Buffer(&sVertexBlock);
}

void CGraphics::UpdatePixelBlock()
{
    mpPixelBlockBuffer->Buffer(&sPixelBlock);
}

void CGraphics::UpdateLightBlock()
{
    mpLightBlockBuffer->Buffer(&sLightBlock);
}

GLuint CGraphics::MVPBlockBindingPoint()
{
    return 0;
}

GLuint CGraphics::VertexBlockBindingPoint()
{
    return 1;
}

GLuint CGraphics::PixelBlockBindingPoint()
{
    return 2;
}

GLuint CGraphics::LightBlockBindingPoint()
{
    return 3;
}

GLuint CGraphics::BoneTransformBlockBindingPoint()
{
    return 4;
}

uint32_t CGraphics::GetContextIndex()
{
    for (uint32_t iCon = 0; iCon < 32; iCon++)
    {
        const auto Mask = (1U << iCon);
        if ((mContextIndices & Mask) == 0)
        {
            mContextIndices |= Mask;

            if (mVAMs.size() >= iCon)
                mVAMs.resize(iCon + 1);

            mVAMs[iCon] = std::make_unique<CVertexArrayManager>();
            return iCon;
        }
    }

    return -1;
}

uint32_t CGraphics::GetActiveContext()
{
    return mActiveContext;
}

void CGraphics::ReleaseContext(uint32_t Index)
{
    if (Index < 32)
        mContextIndices &= ~(1U << Index);

    if (mActiveContext == Index)
        mActiveContext = -1;

    mVAMs[Index].reset();
}

void CGraphics::SetActiveContext(uint32_t Index)
{
    mActiveContext = Index;
    mVAMs[Index]->SetCurrent();
    CMaterial::KillCachedMaterial();
    CShader::KillCachedShader();
}

void CGraphics::SetDefaultLighting()
{
    sNumLights = 0; // CLight load function increments the light count by 1, which is why I set it to 0
    sDefaultDirectionalLights[0].Load();
    sDefaultDirectionalLights[1].Load();
    sDefaultDirectionalLights[2].Load();
    sNumLights = 0;
    UpdateLightBlock();

    sVertexBlock.COLOR0_Amb = CColor::Gray();
    sVertexBlock.COLOR0_Mat = CColor::White();
    UpdateVertexBlock();
}

void CGraphics::SetupAmbientColor()
{
    if (sLightMode == ELightingMode::World)
        sVertexBlock.COLOR0_Amb = sAreaAmbientColor * sWorldLightMultiplier;
    else if (sLightMode == ELightingMode::Basic)
        sVertexBlock.COLOR0_Amb = skDefaultAmbientColor;
    else
        sVertexBlock.COLOR0_Amb = CColor::TransparentWhite();
}

void CGraphics::SetIdentityMVP()
{
    sMVPBlock.ModelMatrix = CMatrix4f::Identity();
    sMVPBlock.ViewMatrix = CMatrix4f::Identity();
    sMVPBlock.ProjectionMatrix = CMatrix4f::Identity();
}

void CGraphics::LoadBoneTransforms(const CBoneTransformData& rkData)
{
    mpBoneTransformBuffer->BufferRange(rkData.Data(), 0, rkData.DataSize());
    mIdentityBoneTransforms = false;
}

void CGraphics::LoadIdentityBoneTransforms()
{
    static constexpr CTransform4f skIdentityTransforms[sNumBoneTransforms];

    if (!mIdentityBoneTransforms)
    {
        mpBoneTransformBuffer->Buffer(&skIdentityTransforms);
        mIdentityBoneTransforms = true;
    }
}
