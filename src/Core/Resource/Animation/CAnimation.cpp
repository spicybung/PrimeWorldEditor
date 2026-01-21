#include "Core/Resource/Animation/CAnimation.h"

#include <Common/Math/CQuaternion.h>
#include <Common/Math/MathUtil.h>
#include <Common/Math/CVector3f.h>
#include "Core/Resource/Animation/CAnimEventData.h"

#include <cfloat>

CAnimation::CAnimation(CResourceEntry *pEntry /*= 0*/)
    : CResource(pEntry)
{
}

CAnimation::~CAnimation() = default;

std::unique_ptr<CDependencyTree> CAnimation::BuildDependencyTree()
{
    auto pTree = std::make_unique<CDependencyTree>();
    pTree->AddDependency(mpEventData);
    return pTree;
}

void CAnimation::EvaluateTransform(float Time, uint32_t BoneID, CVector3f *pOutTranslation, CQuaternion *pOutRotation, CVector3f *pOutScale) const
{
    const bool kInterpolate = true;
    if (!pOutTranslation && !pOutRotation && !pOutScale)
        return;
    if (mDuration == 0.f)
        return;

    if (Time >= mDuration)
        Time = mDuration;
    if (Time >= FLT_EPSILON)
        Time -= FLT_EPSILON;

    float t = fmodf(Time, mTickInterval) / mTickInterval;
    auto LowKey = uint32_t(Time / mTickInterval);
    if (LowKey == (mNumKeys - 1))
        LowKey = mNumKeys - 2;

    uint8_t ScaleChannel = mBoneInfo[BoneID].ScaleChannelIdx;
    uint8_t RotChannel = mBoneInfo[BoneID].RotationChannelIdx;
    uint8_t TransChannel = mBoneInfo[BoneID].TranslationChannelIdx;

    if (ScaleChannel != 0xFF && pOutScale)
    {
        const CVector3f& rkLow = mScaleChannels[ScaleChannel][LowKey];
        const CVector3f& rkHigh = mScaleChannels[ScaleChannel][LowKey + 1];
        *pOutScale = (kInterpolate ? Math::Lerp<CVector3f>(rkLow, rkHigh, t) : rkLow);
    }

    if (RotChannel != 0xFF && pOutRotation)
    {
        const CQuaternion& rkLow = mRotationChannels[RotChannel][LowKey];
        const CQuaternion& rkHigh = mRotationChannels[RotChannel][LowKey + 1];
        *pOutRotation = (kInterpolate ? rkLow.Slerp(rkHigh, t) : rkLow);
    }

    if (TransChannel != 0xFF && pOutTranslation)
    {
        const CVector3f& rkLow = mTranslationChannels[TransChannel][LowKey];
        const CVector3f& rkHigh = mTranslationChannels[TransChannel][LowKey + 1];
        *pOutTranslation = (kInterpolate ? Math::Lerp<CVector3f>(rkLow, rkHigh, t) : rkLow);
    }
}

bool CAnimation::HasTranslation(uint32_t BoneID) const
{
    return (mBoneInfo[BoneID].TranslationChannelIdx != 0xFF);
}
