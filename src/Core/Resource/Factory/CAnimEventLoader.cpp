#include "Core/Resource/Factory/CAnimEventLoader.h"

#include "Core/CAudioManager.h"
#include "Core/GameProject/CGameProject.h"
#include "Core/Resource/CAudioGroup.h"
#include "Core/Resource/Animation/CAnimEventData.h"

void CAnimEventLoader::LoadEvents(IInputStream& rEVNT)
{
    const auto Version = rEVNT.ReadU32();
    ASSERT(Version == 1 || Version == 2);

    // Loop Events
    const auto NumLoopEvents = rEVNT.ReadU32();
    for (uint32_t iLoop = 0; iLoop < NumLoopEvents; iLoop++)
    {
        LoadLoopEvent(rEVNT);
    }

    // User Events
    const auto NumUserEvents = rEVNT.ReadU32();
    for (uint32_t iUser = 0; iUser < NumUserEvents; iUser++)
    {
        LoadUserEvent(rEVNT);
    }

    // Effect Events
    const auto NumEffectEvents = rEVNT.ReadU32();
    for (uint32_t iFX = 0; iFX < NumEffectEvents; iFX++)
    {
        LoadEffectEvent(rEVNT);
    }

    // Sound Events
    if (Version == 2)
    {
        const auto NumSoundEvents = rEVNT.ReadU32();
        for (uint32_t iSound = 0; iSound < NumSoundEvents; iSound++)
        {
            LoadSoundEvent(rEVNT);
        }
    }
}

int CAnimEventLoader::LoadEventBase(IInputStream& rEVNT)
{
    rEVNT.Skip(0x2);
    rEVNT.ReadString();
    rEVNT.Skip(mGame < EGame::CorruptionProto ? 0x13 : 0x17);
    const auto CharacterIndex = rEVNT.ReadS32();
    rEVNT.Skip(mGame < EGame::CorruptionProto ? 0x4 : 0x18);
    return CharacterIndex;
}

void CAnimEventLoader::LoadLoopEvent(IInputStream& rEVNT)
{
    LoadEventBase(rEVNT);
    rEVNT.Skip(0x1);
}

void CAnimEventLoader::LoadUserEvent(IInputStream& rEVNT)
{
    LoadEventBase(rEVNT);
    rEVNT.Skip(0x4);
    rEVNT.ReadString();
}

void CAnimEventLoader::LoadEffectEvent(IInputStream& rEVNT)
{
    const int CharIndex = LoadEventBase(rEVNT);
    rEVNT.Skip(mGame < EGame::CorruptionProto ? 0x8 : 0x4);
    const CAssetID ParticleID(rEVNT, mGame);
    mpEventData->AddEvent(CharIndex, ParticleID);

    if (mGame <= EGame::Prime)
        rEVNT.ReadString();
    else if (mGame <= EGame::Echoes)
        rEVNT.Skip(0x4);

    rEVNT.Skip(0x8);
}

void CAnimEventLoader::LoadSoundEvent(IInputStream& rEVNT)
{
    const auto CharIndex = LoadEventBase(rEVNT);

    // Metroid Prime 1/2
    if (mGame <= EGame::Echoes)
    {
        const auto SoundID = rEVNT.ReadU32() & 0xFFFF;
        rEVNT.Skip(0x8);
        if (mGame >= EGame::Echoes)
            rEVNT.Skip(0xC);

        if (SoundID != 0xFFFF)
        {
            const SSoundInfo SoundInfo = mResourceStore->Project()->AudioManager()->GetSoundInfo(SoundID);

            if (SoundInfo.pAudioGroup)
                mpEventData->AddEvent(CharIndex, SoundInfo.pAudioGroup->ID());
        }
    }
    else // Metroid Prime 3
    {
        const CAssetID SoundID(rEVNT, mGame);
        mpEventData->AddEvent(CharIndex, SoundID);
        rEVNT.Skip(0x8);

        for (uint32_t StructIdx = 0; StructIdx < 2; StructIdx++)
        {
            const auto StructType = rEVNT.ReadU32();
            ASSERT(StructType <= 2);

            if (StructType == 1)
            {
                rEVNT.Skip(4);
            }
            else if (StructType == 2)
            {
                // This is a maya spline
                rEVNT.Skip(2);
                const auto KnotCount = rEVNT.ReadU32();
                rEVNT.Skip(0xA * KnotCount);
                rEVNT.Skip(9);
            }
        }
    }
}

// ************ STATIC ************
std::unique_ptr<CAnimEventData> CAnimEventLoader::LoadEVNT(IInputStream& rEVNT, CResourceEntry *pEntry)
{
    auto ptr = std::make_unique<CAnimEventData>(pEntry);

    CAnimEventLoader Loader;
    Loader.mpEventData = ptr.get();
    Loader.mGame = EGame::Prime;
    Loader.mResourceStore = pEntry->ResourceStore();
    Loader.LoadEvents(rEVNT);

    return ptr;
}

std::unique_ptr<CAnimEventData> CAnimEventLoader::LoadAnimSetEvents(IInputStream& rANCS, CResourceStore* resourceStore)
{
    auto ptr = std::make_unique<CAnimEventData>();

    CAnimEventLoader Loader;
    Loader.mpEventData = ptr.get();
    Loader.mGame = EGame::Echoes;
    Loader.mResourceStore = resourceStore;
    Loader.LoadEvents(rANCS);

    return ptr;
}

std::unique_ptr<CAnimEventData> CAnimEventLoader::LoadCorruptionCharacterEventSet(IInputStream& rCHAR, CResourceStore* resourceStore)
{
    auto ptr = std::make_unique<CAnimEventData>();

    CAnimEventLoader Loader;
    Loader.mpEventData = ptr.get();
    Loader.mGame = EGame::Corruption;
    Loader.mResourceStore = resourceStore;

    // Read event set header
    rCHAR.Skip(0x4); // Skip animation ID
    rCHAR.ReadString(); // Skip set name

    // Read effect events
    const auto NumEffectEvents = rCHAR.ReadU32();
    for (uint32_t EventIdx = 0; EventIdx < NumEffectEvents; EventIdx++)
    {
        rCHAR.ReadString();
        Loader.LoadEffectEvent(rCHAR);
    }

    // Read sound events
    const auto NumSoundEvents = rCHAR.ReadU32();
    for (uint32_t EventIdx = 0; EventIdx < NumSoundEvents; EventIdx++)
    {
        rCHAR.ReadString();
        Loader.LoadSoundEvent(rCHAR);
    }

    return ptr;
}
