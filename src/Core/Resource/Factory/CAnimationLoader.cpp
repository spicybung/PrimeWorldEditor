#include "Core/Resource/Factory/CAnimationLoader.h"

#include <Common/Log.h>
#include <Common/Macros.h>
#include <Common/FileIO/CBitStreamInWrapper.h>
#include <Common/Math/CQuaternion.h>
#include <Common/Math/MathUtil.h>
#include "Core/NRangeUtils.h"
#include "Core/Resource/Animation/CAnimation.h"
#include "Core/Resource/Animation/CAnimEventData.h"

#include <cmath>

bool CAnimationLoader::UncompressedCheckEchoes()
{
    // The best way we have to tell this is an Echoes ANIM is to try to parse it as an
    // Echoes ANIM and see whether we read the file correctly. The formatting has to be
    // a little weird because we have to make sure we don't try to seek or read anything
    // past the end of the file. The +4 being added to each size we test is to account
    // for the next size value of the next array.
    const auto End = mpInput->Size();

    const auto NumRotIndices = mpInput->ReadU32();
    if (mpInput->Tell() + NumRotIndices + 4 >= End) return false;
    mpInput->Seek(NumRotIndices, SEEK_CUR);

    const auto NumTransIndices = mpInput->ReadU32();
    if (mpInput->Tell() + NumTransIndices + 4 >= End) return false;
    mpInput->Seek(NumTransIndices, SEEK_CUR);

    const auto NumScaleIndices = mpInput->ReadU32();
    if (mpInput->Tell() + NumScaleIndices + 4 >= End) return false;
    mpInput->Seek(NumScaleIndices, SEEK_CUR);

    const auto ScaleKeysSize = mpInput->ReadU32() * 0xC;
    if (mpInput->Tell() + ScaleKeysSize + 4 >= End) return false;
    mpInput->Seek(ScaleKeysSize, SEEK_CUR);

    const auto RotKeysSize = mpInput->ReadU32() * 0x10;
    if (mpInput->Tell() + RotKeysSize + 4 >= End) return false;
    mpInput->Seek(RotKeysSize, SEEK_CUR);

    const auto TransKeysSize = mpInput->ReadU32() * 0xC;
    return (mpInput->Tell() + TransKeysSize == End);
}

EGame CAnimationLoader::UncompressedCheckVersion()
{
    // Call this function after the bone channel index array
    // No version number, so this is how we have to determine the version...
    const auto Start = mpInput->Tell();
    const bool Echoes = UncompressedCheckEchoes();
    mpInput->Seek(Start, SEEK_SET);
    return (Echoes ? EGame::Echoes : EGame::Prime);
}

void CAnimationLoader::ReadUncompressedANIM()
{
    mpAnim->mDuration = mpInput->ReadF32();
    mpInput->Seek(0x4, SEEK_CUR); // Skip differential state
    mpAnim->mTickInterval = mpInput->ReadF32();
    mpInput->Seek(0x4, SEEK_CUR); // Skip differential state

    mpAnim->mNumKeys = mpInput->ReadU32();
    mpInput->Seek(0x4, SEEK_CUR); // Skip root bone ID

    // Read bone channel info
    uint32_t NumBoneChannels = 0;
    uint32_t NumScaleChannels = 0;
    uint32_t NumRotationChannels = 0;
    uint32_t NumTranslationChannels = 0;

    // Bone channel list
    const auto NumBoneIndices = mpInput->ReadU32();
    ASSERT(NumBoneIndices == 100);
    std::vector<uint8_t> BoneIndices(NumBoneIndices);

    for (auto& index : BoneIndices)
    {
        index = mpInput->ReadU8();

        if (index != 0xFF)
            NumBoneChannels++;
    }

    if (mGame == EGame::Invalid)
        mGame = UncompressedCheckVersion();

    // Echoes only - rotation channel indices
    std::vector<uint8_t> RotationIndices;

    if (mGame >= EGame::EchoesDemo)
    {
        const auto NumRotationIndices = mpInput->ReadU32();
        RotationIndices.resize(NumRotationIndices);

        for (auto& index : RotationIndices)
        {
            index = mpInput->ReadU8();

            if (index != 0xFF)
                NumRotationChannels++;
        }
    }
    else
    {
        // In MP1 every bone channel has a rotation, so just copy the valid channels from the bone channel list.
        RotationIndices.resize(NumBoneChannels);

        for (const auto index : BoneIndices)
        {
            if (index != 0xFF)
            {
                RotationIndices[NumRotationChannels] = index;
                NumRotationChannels++;
            }
        }
    }

    // Translation channel indices
    const auto NumTransIndices = mpInput->ReadU32();
    std::vector<uint8_t> TransIndices(NumTransIndices);

    for (auto& index : TransIndices)
    {
        index = mpInput->ReadU8();

        if (index != 0xFF)
            NumTranslationChannels++;
    }

    // Echoes only - scale channel indices
    std::vector<uint8_t> ScaleIndices;

    if (mGame >= EGame::EchoesDemo)
    {
        const auto NumScaleIndices = mpInput->ReadU32();
        ScaleIndices.resize(NumScaleIndices);

        for (auto& index : ScaleIndices)
        {
            index = mpInput->ReadU8();

            if (index != 0xFF)
                NumScaleChannels++;
        }
    }

    // Set up bone channel info
    for (size_t iBone = 0, iChan = 0; iBone < NumBoneIndices; iBone++)
    {
        const uint8_t BoneIdx = BoneIndices[iBone];

        if (BoneIdx != 0xFF)
        {
            mpAnim->mBoneInfo[iBone].TranslationChannelIdx = (TransIndices.empty() ? 0xFF : TransIndices[iChan]);
            mpAnim->mBoneInfo[iBone].RotationChannelIdx = (RotationIndices.empty() ? 0xFF : RotationIndices[iChan]);
            mpAnim->mBoneInfo[iBone].ScaleChannelIdx = (ScaleIndices.empty() ? 0xFF : ScaleIndices[iChan]);
            iChan++;
        }
        else
        {
            mpAnim->mBoneInfo[iBone].TranslationChannelIdx = 0xFF;
            mpAnim->mBoneInfo[iBone].RotationChannelIdx = 0xFF;
            mpAnim->mBoneInfo[iBone].ScaleChannelIdx = 0xFF;
        }
    }

    // Read bone transforms
    if (mGame >= EGame::EchoesDemo)
    {
        mpInput->Seek(0x4, SEEK_CUR); // Skipping scale key count
        mpAnim->mScaleChannels.resize(NumScaleChannels);

        for (auto& scaleChannel : mpAnim->mScaleChannels)
        {
            scaleChannel.reserve(mpAnim->mNumKeys);
            for (size_t iKey = 0; iKey < mpAnim->mNumKeys; iKey++)
                scaleChannel.emplace_back(*mpInput);
        }
    }

    mpInput->Seek(0x4, SEEK_CUR); // Skipping rotation key count
    mpAnim->mRotationChannels.resize(NumRotationChannels);

    for (auto& rotationChannel : mpAnim->mRotationChannels)
    {
        rotationChannel.reserve(mpAnim->mNumKeys);
        for (size_t iKey = 0; iKey < mpAnim->mNumKeys; iKey++)
            rotationChannel.emplace_back(*mpInput);
    }

    mpInput->Seek(0x4, SEEK_CUR); // Skipping translation key count
    mpAnim->mTranslationChannels.resize(NumTranslationChannels);

    for (auto& transChannel : mpAnim->mTranslationChannels)
    {
        transChannel.reserve(mpAnim->mNumKeys);
        for (size_t iKey = 0; iKey < mpAnim->mNumKeys; iKey++)
            transChannel.emplace_back(*mpInput);
    }

    if (mGame == EGame::Prime)
    {
        mpAnim->mpEventData = gpResourceStore->LoadResource<CAnimEventData>(CAssetID(mpInput->ReadU32()));
    }
}

void CAnimationLoader::ReadCompressedANIM()
{
    // Header
    mpInput->Seek(0x4, SEEK_CUR); // Skip alloc size

    // Version check
    mGame = (mpInput->PeekS16() == 0x0101 ? EGame::Echoes : EGame::Prime);

    // Check the ANIM resource's game instead of the version check we just determined.
    // The Echoes demo has some ANIMs that use MP1's format, but don't have the EVNT reference.
    if (mpAnim->Game() <= EGame::Prime)
    {
        mpAnim->mpEventData = gpResourceStore->LoadResource<CAnimEventData>(CAssetID(mpInput->ReadU32()));
    }

    mpInput->Seek(mGame <= EGame::Prime ? 4 : 2, SEEK_CUR); // Skip unknowns
    mpAnim->mDuration = mpInput->ReadF32();
    mpAnim->mTickInterval = mpInput->ReadF32();
    mpInput->Seek(0x8, SEEK_CUR); // Skip two unknown values

    mRotationDivisor = mpInput->ReadU32();
    mTranslationMultiplier = mpInput->ReadF32();
    if (mGame >= EGame::EchoesDemo)
        mScaleMultiplier = mpInput->ReadF32();
    const auto NumBoneChannels = mpInput->ReadU32();
    mpInput->Seek(0x4, SEEK_CUR); // Skip unknown value

    // Read key flags
    const auto NumKeys = mpInput->ReadU32();
    mpAnim->mNumKeys = NumKeys;
    mKeyFlags.resize(NumKeys);
    {
        CBitStreamInWrapper BitStream(mpInput);

        for (auto&& flag : mKeyFlags)
            flag = BitStream.ReadBit();
    }
    mpInput->Seek(mGame == EGame::Prime ? 0x8 : 0x4, SEEK_CUR);

    // Read bone channel descriptors
    mCompressedChannels.resize(NumBoneChannels);
    mpAnim->mScaleChannels.resize(NumBoneChannels);
    mpAnim->mRotationChannels.resize(NumBoneChannels);
    mpAnim->mTranslationChannels.resize(NumBoneChannels);

    for (size_t iChan = 0; iChan < NumBoneChannels; iChan++)
    {
        SCompressedChannel& rChan = mCompressedChannels[iChan];
        rChan.BoneID = (mGame == EGame::Prime ? mpInput->ReadU32() : mpInput->ReadU8());

        // Read rotation parameters
        rChan.NumRotationKeys = mpInput->ReadU16();

        if (rChan.NumRotationKeys > 0)
        {
            for (size_t iComp = 0; iComp < 3; iComp++)
            {
                rChan.Rotation[iComp] = mpInput->ReadS16();
                rChan.RotationBits[iComp] = mpInput->ReadU8();
            }

            mpAnim->mBoneInfo[rChan.BoneID].RotationChannelIdx = static_cast<uint8_t>(iChan);
        }
        else
        {
            mpAnim->mBoneInfo[rChan.BoneID].RotationChannelIdx = 0xFF;
        }

        // Read translation parameters
        rChan.NumTranslationKeys = mpInput->ReadU16();

        if (rChan.NumTranslationKeys > 0)
        {
            for (size_t iComp = 0; iComp < 3; iComp++)
            {
                rChan.Translation[iComp] = mpInput->ReadS16();
                rChan.TranslationBits[iComp] = mpInput->ReadU8();
            }

            mpAnim->mBoneInfo[rChan.BoneID].TranslationChannelIdx = static_cast<uint8_t>(iChan);
        }
        else
        {
            mpAnim->mBoneInfo[rChan.BoneID].TranslationChannelIdx = 0xFF;
        }

        // Read scale parameters
        uint8_t ScaleIdx = 0xFF;

        if (mGame >= EGame::EchoesDemo)
        {
            rChan.NumScaleKeys = mpInput->ReadU16();

            if (rChan.NumScaleKeys > 0)
            {
                for (size_t iComp = 0; iComp < 3; iComp++)
                {
                    rChan.Scale[iComp] = mpInput->ReadS16();
                    rChan.ScaleBits[iComp] = mpInput->ReadU8();
                }

                ScaleIdx = static_cast<uint8_t>(iChan);
            }
        }
        mpAnim->mBoneInfo[rChan.BoneID].ScaleChannelIdx = ScaleIdx;
    }

    // Read animation data
    ReadCompressedAnimationData();
}

void CAnimationLoader::ReadCompressedAnimationData()
{
    CBitStreamInWrapper BitStream(mpInput);

    // Initialize
    for (auto&& [idx, channel] : Utils::enumerate(mCompressedChannels))
    {
        // Set initial rotation/translation/scale
        if (channel.NumRotationKeys > 0)
        {
            mpAnim->mRotationChannels[idx].reserve(channel.NumRotationKeys + 1);
            CQuaternion Rotation = DequantizeRotation(false, channel.Rotation[0], channel.Rotation[1], channel.Rotation[2]);
            mpAnim->mRotationChannels[idx].push_back(Rotation);
        }

        if (channel.NumTranslationKeys > 0)
        {
            mpAnim->mTranslationChannels[idx].reserve(channel.NumTranslationKeys + 1);
            CVector3f Translate = CVector3f(channel.Translation[0], channel.Translation[1], channel.Translation[2]) * mTranslationMultiplier;
            mpAnim->mTranslationChannels[idx].push_back(Translate);
        }

        if (channel.NumScaleKeys > 0)
        {
            mpAnim->mScaleChannels[idx].reserve(channel.NumScaleKeys + 1);
            CVector3f Scale = CVector3f(channel.Scale[0], channel.Scale[1], channel.Scale[2]) * mScaleMultiplier;
            mpAnim->mScaleChannels[idx].push_back(Scale);
        }
    }

    // Read keys
    for (size_t iKey = 0; iKey < mpAnim->mNumKeys - 1; iKey++)
    {
        const bool KeyPresent = mKeyFlags[iKey + 1];

        for (auto&& [idx, channel] : Utils::enumerate(mCompressedChannels))
        {
            // Read rotation
            if (channel.NumRotationKeys > 0)
            {
                // Note if KeyPresent is false, this isn't the correct value of WSign.
                // However, we're going to recreate this key later via interpolation, so it doesn't matter what value we use here.
                const bool WSign = (KeyPresent ? BitStream.ReadBit() : false);

                if (KeyPresent)
                {
                    channel.Rotation[0] += static_cast<int16_t>(BitStream.ReadBits(channel.RotationBits[0]));
                    channel.Rotation[1] += static_cast<int16_t>(BitStream.ReadBits(channel.RotationBits[1]));
                    channel.Rotation[2] += static_cast<int16_t>(BitStream.ReadBits(channel.RotationBits[2]));
                }

                const CQuaternion Rotation = DequantizeRotation(WSign, channel.Rotation[0], channel.Rotation[1], channel.Rotation[2]);
                mpAnim->mRotationChannels[idx].push_back(Rotation);
            }

            // Read translation
            if (channel.NumTranslationKeys > 0)
            {
                if (KeyPresent)
                {
                    channel.Translation[0] += static_cast<int16_t>(BitStream.ReadBits(channel.TranslationBits[0]));
                    channel.Translation[1] += static_cast<int16_t>(BitStream.ReadBits(channel.TranslationBits[1]));
                    channel.Translation[2] += static_cast<int16_t>(BitStream.ReadBits(channel.TranslationBits[2]));
                }

                const CVector3f Translate = CVector3f(channel.Translation[0], channel.Translation[1], channel.Translation[2]) * mTranslationMultiplier;
                mpAnim->mTranslationChannels[idx].push_back(Translate);
            }

            // Read scale
            if (channel.NumScaleKeys > 0)
            {
                if (KeyPresent)
                {
                    channel.Scale[0] += static_cast<int16_t>(BitStream.ReadBits(channel.ScaleBits[0]));
                    channel.Scale[1] += static_cast<int16_t>(BitStream.ReadBits(channel.ScaleBits[1]));
                    channel.Scale[2] += static_cast<int16_t>(BitStream.ReadBits(channel.ScaleBits[2]));
                }

                const CVector3f Scale = CVector3f(channel.Scale[0], channel.Scale[1], channel.Scale[2]) * mScaleMultiplier;
                mpAnim->mScaleChannels[idx].push_back(Scale);
            }
        }
    }

    // Fill in missing keys
    uint32_t NumMissedKeys = 0;

    for (size_t iKey = 0; iKey < mpAnim->mNumKeys; iKey++)
    {
        if (!mKeyFlags[iKey])
        {
            NumMissedKeys++;
        }
        else if (NumMissedKeys > 0)
        {
            const uint32_t FirstIndex = iKey - NumMissedKeys - 1;
            const size_t LastIndex = iKey;
            const uint32_t RelLastIndex = LastIndex - FirstIndex;

            for (size_t iMissed = 0; iMissed < NumMissedKeys; iMissed++)
            {
                const size_t KeyIndex = FirstIndex + iMissed + 1;
                const size_t RelKeyIndex = (KeyIndex - FirstIndex);
                const float Interp = static_cast<float>(RelKeyIndex) / static_cast<float>(RelLastIndex);

                for (auto&& [idx, channel] : Utils::enumerate(mCompressedChannels))
                {
                    const bool HasTranslationKeys = channel.NumTranslationKeys > 0;
                    const bool HasRotationKeys = channel.NumRotationKeys > 0;
                    const bool HasScaleKeys = channel.NumScaleKeys > 0;

                    if (HasRotationKeys)
                    {
                        const CQuaternion& Left = mpAnim->mRotationChannels[idx][FirstIndex];
                        const CQuaternion& Right = mpAnim->mRotationChannels[idx][LastIndex];
                        mpAnim->mRotationChannels[idx][KeyIndex] = Left.Slerp(Right, Interp);
                    }

                    if (HasTranslationKeys)
                    {
                        const CVector3f& Left = mpAnim->mTranslationChannels[idx][FirstIndex];
                        const CVector3f& Right = mpAnim->mTranslationChannels[idx][LastIndex];
                        mpAnim->mTranslationChannels[idx][KeyIndex] = Math::Lerp<CVector3f>(Left, Right, Interp);
                    }

                    if (HasScaleKeys)
                    {
                        const CVector3f& Left = mpAnim->mScaleChannels[idx][FirstIndex];
                        const CVector3f& Right = mpAnim->mScaleChannels[idx][LastIndex];
                        mpAnim->mScaleChannels[idx][KeyIndex] = Math::Lerp<CVector3f>(Left, Right, Interp);
                    }
                }
            }

            NumMissedKeys = 0;
        }
    }
}

CQuaternion CAnimationLoader::DequantizeRotation(bool Sign, int16_t X, int16_t Y, int16_t Z) const
{
    const float Multiplier = Math::skHalfPi / static_cast<float>(mRotationDivisor);

    CQuaternion Out;
    Out.X = std::sin(static_cast<float>(X) * Multiplier);
    Out.Y = std::sin(static_cast<float>(Y) * Multiplier);
    Out.Z = std::sin(static_cast<float>(Z) * Multiplier);
    Out.W = std::sqrt(std::fmax(1.f - ((Out.X * Out.X) + (Out.Y * Out.Y) + (Out.Z * Out.Z)), 0.f));

    if (Sign)
        Out.W = -Out.W;

    return Out;
}

// ************ STATIC ************
std::unique_ptr<CAnimation> CAnimationLoader::LoadANIM(IInputStream& rANIM, CResourceEntry *pEntry)
{
    // MP3/DKCR unsupported
    if (pEntry->Game() > EGame::Echoes)
        return std::make_unique<CAnimation>(pEntry);

    const auto CompressionType = rANIM.ReadU32();
    if (CompressionType != 0 && CompressionType != 2)
    {
        NLog::Error("{}: Unknown ANIM compression type: {}", rANIM.GetSourceString(), CompressionType);
        return nullptr;
    }

    auto ptr = std::make_unique<CAnimation>(pEntry);

    CAnimationLoader Loader;
    Loader.mpAnim = ptr.get();
    Loader.mGame = pEntry->Game();
    Loader.mpInput = &rANIM;

    if (CompressionType == 0)
        Loader.ReadUncompressedANIM();
    else
        Loader.ReadCompressedANIM();

    return ptr;
}
