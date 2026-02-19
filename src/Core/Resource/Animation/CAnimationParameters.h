#ifndef CANIMATIONPARAMETERS_H
#define CANIMATIONPARAMETERS_H

#include "Core/Resource/TResPtr.h"
#include <Common/EGame.h>
#include <cstdint>

class CModel;
class CResourceEntry;

class CAnimationParameters
{
    EGame mGame = EGame::Prime;
    CAssetID mCharacterID;

    uint32_t mCharIndex = 0;
    uint32_t mAnimIndex = 0;
    uint32_t mUnknown2 = 0;
    uint32_t mUnknown3 = 0;

public:
    CAnimationParameters();
    explicit CAnimationParameters(EGame Game);
    explicit CAnimationParameters(IInputStream& rSCLY, EGame Game);

    CAnimationParameters(const CAnimationParameters&) = default;
    CAnimationParameters& operator=(const CAnimationParameters&) = default;

    void Write(IOutputStream& rSCLY);
    void Serialize(IArchive& rArc);

    const SSetCharacter* GetCurrentSetCharacter(int32_t NodeIndex = -1) const;
    CModel* GetCurrentModel(int32_t NodeIndex = -1);
    TString GetCurrentCharacterName(int32_t NodeIndex = -1) const;

    // Accessors
    EGame Version() const             { return mGame; }
    const CAssetID& ID() const        { return mCharacterID; }
    CAnimSet* AnimSet() const         { return (CAnimSet*) gpResourceStore->LoadResource(mCharacterID); }
    uint32_t CharacterIndex() const   { return mCharIndex; }
    uint32_t AnimIndex() const        { return mAnimIndex; }
    void SetCharIndex(uint32_t Index) { mCharIndex = Index; }
    void SetAnimIndex(uint32_t Index) { mAnimIndex = Index; }

    void SetGame(EGame Game)
    {
        mGame = Game;

        if (!mCharacterID.IsValid())
        {
            mCharacterID = CAssetID::InvalidID(mGame);
        }
        else
        {
            ASSERT( mCharacterID.Length() == CAssetID::GameIDLength(mGame) );
        }
    }

    uint32_t Unknown(uint32_t Index) const;
    void SetResource(const CResourceEntry* entry);
    void SetUnknown(uint32_t Index, uint32_t Value);

    bool operator==(const CAnimationParameters&) const = default;
};

#endif // CANIMATIONPARAMETERS_H
