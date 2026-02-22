#ifndef CANIMATIONLOADER_H
#define CANIMATIONLOADER_H

#include "Core/Resource/TResPtr.h"

#include <Common/EGame.h>
#include <array>
#include <cstdint>
#include <memory>

class CAnimation;
class CQuaternion;
class CResourceEntry;
class IInputStream;

class CAnimationLoader
{
    TResPtr<CAnimation> mpAnim;
    IInputStream *mpInput = nullptr;
    EGame mGame{};

    // Compression data
    std::vector<bool> mKeyFlags;
    float mTranslationMultiplier = 0.0f;
    uint32_t mRotationDivisor = 0;
    float mScaleMultiplier = 0.0f;

    struct SCompressedChannel
    {
        uint32_t BoneID{};
        uint16_t NumRotationKeys{};
        std::array<int16_t, 3> Rotation{};
        std::array<uint8_t, 3> RotationBits{};
        uint16_t NumTranslationKeys{};
        std::array<int16_t, 3> Translation{};
        std::array<uint8_t, 3> TranslationBits{};
        uint16_t NumScaleKeys{};
        std::array<int16_t, 3> Scale{};
        std::array<uint8_t, 3> ScaleBits{};
    };
    std::vector<SCompressedChannel> mCompressedChannels;

    CAnimationLoader() = default;
    bool UncompressedCheckEchoes();
    EGame UncompressedCheckVersion();
    void ReadUncompressedANIM();
    void ReadCompressedANIM();
    void ReadCompressedAnimationData();
    CQuaternion DequantizeRotation(bool Sign, int16_t X, int16_t Y, int16_t Z) const;

public:
    static std::unique_ptr<CAnimation> LoadANIM(IInputStream& rANIM, CResourceEntry *pEntry);
};

#endif // CANIMATIONLOADER_H
