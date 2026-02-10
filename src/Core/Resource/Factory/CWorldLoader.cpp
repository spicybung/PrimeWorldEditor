#include "Core/Resource/Factory/CWorldLoader.h"

#include "Core/GameProject/CGameInfo.h"
#include "Core/GameProject/CGameProject.h"
#include "Core/GameProject/CResourceStore.h"
#include "Core/Resource/CWorld.h"

#include <Common/Log.h>

CWorldLoader::CWorldLoader() = default;

void CWorldLoader::LoadPrimeMLVL(IInputStream& rMLVL)
{
    /*
     * This function loads MLVL files from Prime 1/2
     * We start immediately after the "version" value (0x8 in the file)
     */
    // Header
    if (mVersion < EGame::CorruptionProto)
    {
        mpWorld->mpWorldName = gpResourceStore->LoadResource(CAssetID(rMLVL.ReadU32()), EResourceType::StringTable);

        if (mVersion == EGame::Echoes)
            mpWorld->mpDarkWorldName = gpResourceStore->LoadResource(CAssetID(rMLVL.ReadU32()), EResourceType::StringTable);

        if (mVersion >= EGame::Echoes)
            mpWorld->mTempleKeyWorldIndex = rMLVL.ReadU32();

        if (mVersion >= EGame::Prime)
            mpWorld->mpSaveWorld = gpResourceStore->LoadResource(CAssetID(rMLVL.ReadU32()), EResourceType::SaveWorld);

        mpWorld->mpDefaultSkybox = gpResourceStore->LoadResource(CAssetID(rMLVL.ReadU32()), EResourceType::Model);
    }
    else
    {
        mpWorld->mpWorldName = gpResourceStore->LoadResource(CAssetID(rMLVL.ReadU64()), EResourceType::StringTable);
        rMLVL.Seek(0x4, SEEK_CUR); // Skipping unknown value
        mpWorld->mpSaveWorld = gpResourceStore->LoadResource(CAssetID(rMLVL.ReadU64()), EResourceType::SaveWorld);
        mpWorld->mpDefaultSkybox = gpResourceStore->LoadResource(CAssetID(rMLVL.ReadU64()), EResourceType::Model);
    }

    // Memory relays - only in MP1
    if (mVersion == EGame::Prime)
    {
        const auto NumMemoryRelays = rMLVL.ReadU32();
        mpWorld->mMemoryRelays.reserve(NumMemoryRelays);

        for (uint32_t iMem = 0; iMem < NumMemoryRelays; iMem++)
        {
            auto& MemRelay = mpWorld->mMemoryRelays.emplace_back();
            MemRelay.InstanceID = rMLVL.ReadU32();
            MemRelay.TargetID = rMLVL.ReadU32();
            MemRelay.Message = rMLVL.ReadU16();
            MemRelay.Active = rMLVL.ReadBool();
        }
    }

    // Areas - here's the real meat of the file
    const auto NumAreas = rMLVL.ReadU32();
    if (mVersion == EGame::Prime)
        rMLVL.Seek(0x4, SEEK_CUR);
    mpWorld->mAreas.resize(NumAreas);

    for (auto& area : mpWorld->mAreas)
    {
        // Area header
        area.pAreaName = gpResourceStore->LoadResource<CStringTable>(CAssetID(rMLVL, mVersion));
        area.Transform = CTransform4f(rMLVL);
        area.AetherBox = CAABox(rMLVL);
        area.AreaResID = CAssetID(rMLVL, mVersion);
        area.AreaID = CAssetID(rMLVL, mVersion);

        // Attached areas
        const auto NumAttachedAreas = rMLVL.ReadU32();
        area.AttachedAreaIDs.reserve(NumAttachedAreas);
        for (uint32_t iAttached = 0; iAttached < NumAttachedAreas; iAttached++)
            area.AttachedAreaIDs.push_back(rMLVL.ReadU16());

        // Skip dependency list - this is very fast to regenerate so there's no use in caching it
        if (mVersion < EGame::CorruptionProto)
        {
            rMLVL.Seek(0x4, SEEK_CUR);
            const auto NumDependencies = rMLVL.ReadU32();
            rMLVL.Seek(NumDependencies * 8, SEEK_CUR);

            const auto NumDependencyOffsets = rMLVL.ReadU32();
            rMLVL.Seek(NumDependencyOffsets * 4, SEEK_CUR);
        }

        // Docks
        const auto NumDocks = rMLVL.ReadU32();
        area.Docks.resize(NumDocks);

        for (auto& dock : area.Docks)
        {
            const auto NumConnectingDocks = rMLVL.ReadU32();
            dock.ConnectingDocks.resize(NumConnectingDocks);

            for (auto& ConnectingDock : dock.ConnectingDocks)
            {
                ConnectingDock.AreaIndex = rMLVL.ReadU32();
                ConnectingDock.DockIndex = rMLVL.ReadU32();
            }

            const auto NumCoordinates = rMLVL.ReadU32();
            ASSERT(NumCoordinates == 4);
            dock.DockCoordinates.resize(NumCoordinates);

            for (auto& coordinate : dock.DockCoordinates)
                coordinate = CVector3f(rMLVL);
        }

        // Rels
        if (mVersion == EGame::EchoesDemo || mVersion == EGame::Echoes)
        {
            const auto NumRels = rMLVL.ReadU32();
            area.RelFilenames.resize(NumRels);

            for (auto& filename : area.RelFilenames)
                filename = rMLVL.ReadString();

            if (mVersion == EGame::Echoes)
            {
                const auto NumRelOffsets = rMLVL.ReadU32(); // Don't know what these offsets correspond to
                area.RelOffsets.resize(NumRelOffsets);

                for (auto& offset : area.RelOffsets)
                    offset = rMLVL.ReadU32();
            }
        }

        // Internal name - MP1 doesn't have this, we'll get it from the GameInfo file later
        if (mVersion >= EGame::EchoesDemo)
            area.InternalName = rMLVL.ReadString();
    }

    // MapWorld
    mpWorld->mpMapWorld = gpResourceStore->LoadResource(CAssetID(rMLVL, mVersion), EResourceType::MapWorld);
    rMLVL.Seek(0x5, SEEK_CUR); // Unknown values which are always 0

    // Audio Groups - we don't need this info as we regenerate it on cook
    if (mVersion == EGame::Prime)
    {
        const auto NumAudioGrps = rMLVL.ReadU32();
        rMLVL.Seek(0x8 * NumAudioGrps, SEEK_CUR);
        rMLVL.Seek(0x1, SEEK_CUR); // Unknown values which are always 0
    }

    // Layer flags
    rMLVL.Seek(0x4, SEEK_CUR); // Skipping redundant area count
    for (auto& area : mpWorld->mAreas)
    {
        const auto NumLayers = rMLVL.ReadU32();
        if (NumLayers != area.Layers.size())
            area.Layers.resize(NumLayers);

        const auto LayerFlags = rMLVL.ReadU64();
        for (uint32_t iLayer = 0; iLayer < NumLayers; iLayer++)
            area.Layers[iLayer].Active = (((LayerFlags >> iLayer) & 0x1) == 1);
    }

    // Layer names
    rMLVL.Seek(0x4, SEEK_CUR); // Skipping redundant layer count
    for (auto& area : mpWorld->mAreas)
    {
        for (auto& layer : area.Layers)
            layer.LayerName = rMLVL.ReadString();
    }

    // Layer state IDs
    if (mVersion >= EGame::Corruption)
    {
        rMLVL.Seek(0x4, SEEK_CUR); // Skipping redundant layer count
        for (auto& area : mpWorld->mAreas)
        {
            for (auto& layer : area.Layers)
                layer.LayerStateID = CSavedStateID(rMLVL);
        }
    }

    // Last part of the file is layer name offsets, but we don't need it
}

void CWorldLoader::LoadReturnsMLVL(IInputStream& rMLVL)
{
    mpWorld->mpWorldName = gpResourceStore->LoadResource<CStringTable>(CAssetID(rMLVL.ReadU64()));

    CWorld::STimeAttackData& rData = mpWorld->mTimeAttackData;
    rData.HasTimeAttack = rMLVL.ReadBool();

    if (rData.HasTimeAttack)
    {
        rData.ActNumber = rMLVL.ReadString();
        rData.BronzeTime = rMLVL.ReadF32();
        rData.SilverTime = rMLVL.ReadF32();
        rData.GoldTime = rMLVL.ReadF32();
        rData.ShinyGoldTime = rMLVL.ReadF32();
    }

    mpWorld->mpSaveWorld = gpResourceStore->LoadResource(CAssetID(rMLVL.ReadU64()), EResourceType::SaveWorld);
    mpWorld->mpDefaultSkybox = gpResourceStore->LoadResource<CModel>(CAssetID(rMLVL.ReadU64()));

    // Areas
    const auto NumAreas = rMLVL.ReadU32();
    mpWorld->mAreas.resize(NumAreas);

    for (auto& area : mpWorld->mAreas)
    {
        // Area header
        area.pAreaName = gpResourceStore->LoadResource<CStringTable>(CAssetID(rMLVL.ReadU64()));
        area.Transform = CTransform4f(rMLVL);
        area.AetherBox = CAABox(rMLVL);
        area.AreaResID = CAssetID(rMLVL.ReadU64());
        area.AreaID = CAssetID(rMLVL.ReadU64());

        rMLVL.Seek(0x4, SEEK_CUR);
        area.InternalName = rMLVL.ReadString();
    }

    // Layer flags
    rMLVL.Seek(0x4, SEEK_CUR); // Skipping redundant area count

    for (auto& area : mpWorld->mAreas)
    {
        const auto NumLayers = rMLVL.ReadU32();
        area.Layers.resize(NumLayers);

        const auto LayerFlags = rMLVL.ReadU64();
        for (uint32_t iLayer = 0; iLayer < NumLayers; iLayer++)
            area.Layers[iLayer].Active = (((LayerFlags >> iLayer) & 0x1) == 1);
    }

    // Layer names
    rMLVL.Seek(0x4, SEEK_CUR); // Skipping redundant layer count
    for (auto& area : mpWorld->mAreas)
    {
        for (auto& layer : area.Layers)
            layer.LayerName = rMLVL.ReadString();
    }

    // Layer state IDs
    rMLVL.Seek(0x4, SEEK_CUR); // Skipping redundant layer count
    for (auto& area : mpWorld->mAreas)
    {
        for (auto& layer : area.Layers)
            layer.LayerStateID = CSavedStateID(rMLVL);
    }

    // Last part of the file is layer name offsets, but we don't need it
}

void CWorldLoader::GenerateEditorData()
{
    const CGameInfo *pGameInfo = mpWorld->Entry()->ResourceStore()->Project()->GameInfo();

    if (mVersion > EGame::Prime)
        return;

    for (auto& area : mpWorld->mAreas)
    {
        area.InternalName = pGameInfo->GetAreaName(area.AreaResID);
        ASSERT(!area.InternalName.IsEmpty());
    }
}

std::unique_ptr<CWorld> CWorldLoader::LoadMLVL(IInputStream& rMLVL, CResourceEntry *pEntry)
{
    if (!rMLVL.IsValid())
        return nullptr;

    const auto Magic = rMLVL.ReadU32();
    if (Magic != 0xDEAFBABE)
    {
        NLog::Error("{}: Invalid MLVL magic: 0x{:08X}", rMLVL.GetSourceString(), Magic);
        return nullptr;
    }

    const auto FileVersion = rMLVL.ReadU32();
    const auto Version = GetFormatVersion(FileVersion);
    if (Version == EGame::Invalid)
    {
        NLog::Error("{}: Unsupported MLVL version: 0x{:X}", rMLVL.GetSourceString(), FileVersion);
        return nullptr;
    }

    // Filestream is valid, magic+version are valid; everything seems good!
    auto ptr = std::make_unique<CWorld>(pEntry);

    CWorldLoader Loader;
    Loader.mpWorld = ptr.get();
    Loader.mVersion = Version;

    if (Version != EGame::DKCReturns)
        Loader.LoadPrimeMLVL(rMLVL);
    else
        Loader.LoadReturnsMLVL(rMLVL);

    Loader.GenerateEditorData();
    return ptr;
}

EGame CWorldLoader::GetFormatVersion(uint32_t Version)
{
    switch (Version)
    {
        case 0xD: return EGame::PrimeDemo;
        case 0x11: return EGame::Prime;
        case 0x14: return EGame::EchoesDemo;
        case 0x17: return EGame::Echoes;
        case 0x19: return EGame::Corruption;
        case 0x1B: return EGame::DKCReturns;
        default: return EGame::Invalid;
    }
}
