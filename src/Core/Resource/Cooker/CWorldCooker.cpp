#include "Core/Resource/Cooker/CWorldCooker.h"

#include "Core/GameProject/DependencyListBuilders.h"
#include "Core/Resource/CAudioGroup.h"
#include "Core/Resource/CWorld.h"

CWorldCooker::CWorldCooker() = default;

// ************ STATIC ************
bool CWorldCooker::CookMLVL(CWorld *pWorld, IOutputStream& rMLVL)
{
    ASSERT(rMLVL.IsValid());
    const EGame Game = pWorld->Game();

    // MLVL Header
    rMLVL.WriteU32(0xDEAFBABE);
    rMLVL.WriteU32(GetMLVLVersion(pWorld->Game()));

    const CAssetID WorldNameID = pWorld->mpWorldName != nullptr ? pWorld->mpWorldName->ID() : CAssetID::InvalidID(Game);
    const CAssetID DarkWorldNameID = pWorld->mpDarkWorldName != nullptr ? pWorld->mpDarkWorldName->ID() : CAssetID::InvalidID(Game);
    const CAssetID SaveWorldID = pWorld->mpSaveWorld != nullptr ? pWorld->mpSaveWorld->ID() : CAssetID::InvalidID(Game);
    const CAssetID DefaultSkyID = pWorld->mpDefaultSkybox != nullptr ? pWorld->mpDefaultSkybox->ID() : CAssetID::InvalidID(Game);

    WorldNameID.Write(rMLVL);

    if (Game == EGame::EchoesDemo || Game == EGame::Echoes)
    {
        DarkWorldNameID.Write(rMLVL);
    }
    if (Game >= EGame::EchoesDemo && Game <= EGame::Corruption)
    {
        rMLVL.WriteU32(pWorld->mTempleKeyWorldIndex);
    }
    if (Game == EGame::DKCReturns)
    {
        const CWorld::STimeAttackData& rkData = pWorld->mTimeAttackData;
        rMLVL.WriteBool(rkData.HasTimeAttack);

        if (rkData.HasTimeAttack)
        {
            rMLVL.WriteString(rkData.ActNumber);
            rMLVL.WriteF32(rkData.BronzeTime);
            rMLVL.WriteF32(rkData.SilverTime);
            rMLVL.WriteF32(rkData.GoldTime);
            rMLVL.WriteF32(rkData.ShinyGoldTime);
        }
    }

    SaveWorldID.Write(rMLVL);
    DefaultSkyID.Write(rMLVL);

    // Memory Relays
    if (Game == EGame::Prime)
    {
        rMLVL.WriteU32(static_cast<uint32_t>(pWorld->mMemoryRelays.size()));

        for (const auto& relay : pWorld->mMemoryRelays)
        {
            rMLVL.WriteU32(relay.InstanceID);
            rMLVL.WriteU32(relay.TargetID);
            rMLVL.WriteU16(relay.Message);
            rMLVL.WriteBool(relay.Active);
        }
    }

    // Areas
    rMLVL.WriteU32(static_cast<uint32_t>(pWorld->mAreas.size()));
    if (Game <= EGame::Prime)
        rMLVL.WriteU32(1); // Unknown
    std::set<CAssetID> AudioGroups;

    for (auto& rArea : pWorld->mAreas)
    {
        // Area Header
        CResourceEntry *pAreaEntry = gpResourceStore->FindEntry(rArea.AreaResID);
        ASSERT(pAreaEntry && pAreaEntry->ResourceType() == EResourceType::Area);

        const CAssetID AreaNameID = rArea.pAreaName != nullptr ? rArea.pAreaName->ID() : CAssetID::InvalidID(Game);
        AreaNameID.Write(rMLVL);
        rArea.Transform.Write(rMLVL);
        rArea.AetherBox.Write(rMLVL);
        rArea.AreaResID.Write(rMLVL);
        rArea.AreaID.Write(rMLVL);

        // Attached Areas
        if (Game <= EGame::Corruption)
        {
            rMLVL.WriteU32(static_cast<uint32_t>(rArea.AttachedAreaIDs.size()));

            for (const auto id : rArea.AttachedAreaIDs)
                rMLVL.WriteU16(id);
        }

        // Dependencies
        if (Game <= EGame::Echoes)
        {
            std::list<CAssetID> Dependencies;
            std::list<uint32_t> LayerDependsOffsets;
            CAreaDependencyListBuilder Builder(pAreaEntry);
            Builder.BuildDependencyList(Dependencies, LayerDependsOffsets, &AudioGroups);

            rMLVL.WriteU32(0);
            rMLVL.WriteU32(static_cast<uint32_t>(Dependencies.size()));

            for (const auto& ID : Dependencies)
            {
                CResourceEntry *pEntry = gpResourceStore->FindEntry(ID);
                ID.Write(rMLVL);
                pEntry->CookedExtension().Write(rMLVL);
            }

            rMLVL.WriteU32(static_cast<uint32_t>(LayerDependsOffsets.size()));

            for (const auto offset : LayerDependsOffsets)
                rMLVL.WriteU32(offset);
        }

        // Docks
        if (Game <= EGame::Corruption)
        {
            rMLVL.WriteU32(static_cast<uint32_t>(rArea.Docks.size()));

            for (const auto& dock : rArea.Docks)
            {
                rMLVL.WriteU32(static_cast<uint32_t>(dock.ConnectingDocks.size()));

                for (const auto& connectingDock : dock.ConnectingDocks)
                {
                    rMLVL.WriteU32(connectingDock.AreaIndex);
                    rMLVL.WriteU32(connectingDock.DockIndex);
                }

                rMLVL.WriteU32(static_cast<uint32_t>(dock.DockCoordinates.size()));

                for (const auto& coordinate : dock.DockCoordinates)
                    coordinate.Write(rMLVL);
            }
        }

        // Module Dependencies
        if (Game == EGame::EchoesDemo || Game == EGame::Echoes)
        {
            std::vector<TString> ModuleNames;
            std::vector<uint32_t> ModuleLayerOffsets;
            const auto *pAreaDeps = static_cast<CAreaDependencyTree*>(pAreaEntry->Dependencies());
            pAreaDeps->GetModuleDependencies(Game, ModuleNames, ModuleLayerOffsets);

            rMLVL.WriteU32(static_cast<uint32_t>(ModuleNames.size()));

            for (const auto& name : ModuleNames)
                rMLVL.WriteString(name);

            rMLVL.WriteU32(static_cast<uint32_t>(ModuleLayerOffsets.size()));

            for (const auto offset : ModuleLayerOffsets)
                rMLVL.WriteU32(offset);
        }

        // Unknown
        if (Game == EGame::DKCReturns)
            rMLVL.WriteU32(0);

        // Internal Name
        if (Game >= EGame::EchoesDemo)
            rMLVL.WriteString(rArea.InternalName);
    }

    if (Game <= EGame::Corruption)
    {
        // World Map
        const CAssetID MapWorldID = pWorld->mpMapWorld != nullptr ? pWorld->mpMapWorld->ID() : CAssetID::skInvalidID32;
        MapWorldID.Write(rMLVL);

        // Script Layer - unused in all retail builds but this will need to be supported eventually to properly support the MP1 demo
        rMLVL.WriteU8(0);
        rMLVL.WriteU32(0);
    }

    // Audio Groups
    if (Game <= EGame::Prime)
    {
        // Create sorted list of audio groups (sort by group ID)
        std::vector<CAudioGroup*> SortedAudioGroups;
        SortedAudioGroups.reserve(AudioGroups.size());

        for (const auto AudioGroup : AudioGroups)
        {
            CAudioGroup *pGroup = gpResourceStore->LoadResource<CAudioGroup>(AudioGroup);
            ASSERT(pGroup);
            SortedAudioGroups.push_back(pGroup);
        }

        std::ranges::sort(SortedAudioGroups, [](const auto* pLeft, const auto* pRight) {
            return pLeft->GroupID() < pRight->GroupID();
        });

        // Write sorted audio group list to file
        rMLVL.WriteU32(static_cast<uint32_t>(SortedAudioGroups.size()));

        for (const auto* pGroup : SortedAudioGroups)
        {
            rMLVL.WriteU32(pGroup->GroupID());
            pGroup->ID().Write(rMLVL);
        }

        rMLVL.WriteU8(0);
    }

    // Layers
    rMLVL.WriteU32(pWorld->mAreas.size());
    std::vector<TString> LayerNames;
    std::vector<CSavedStateID> LayerStateIDs;
    std::vector<uint32_t> LayerNameOffsets;

    // Layer Flags
    for (const auto& rArea : pWorld->mAreas)
    {
        LayerNameOffsets.push_back(LayerNames.size());
        rMLVL.WriteU32(static_cast<uint32_t>(rArea.Layers.size()));

        uint64_t LayerActiveFlags = UINT64_MAX;

        for (uint32_t iLyr = 0; iLyr < rArea.Layers.size(); iLyr++)
        {
            const CWorld::SArea::SLayer& rLayer = rArea.Layers[iLyr];
            if (!rLayer.Active)
                LayerActiveFlags &= ~(UINT64_C(1) << iLyr);

            LayerNames.push_back(rLayer.LayerName);
            LayerStateIDs.push_back(rLayer.LayerStateID);
        }

        rMLVL.WriteU64(LayerActiveFlags);
    }

    // Layer Names
    rMLVL.WriteU32(static_cast<uint32_t>(LayerNames.size()));

    for (const auto& name : LayerNames)
        rMLVL.WriteString(name);

    // Layer Saved State IDs
    if (Game >= EGame::Corruption)
    {
        rMLVL.WriteU32(static_cast<uint32_t>(LayerStateIDs.size()));

        for (auto& id : LayerStateIDs)
            id.Write(rMLVL);
    }

    // Layer Name Offsets
    rMLVL.WriteU32(static_cast<uint32_t>(LayerNameOffsets.size()));

    for (const auto offset : LayerNameOffsets)
        rMLVL.WriteU32(offset);

    return true;
}

uint32_t CWorldCooker::GetMLVLVersion(EGame Version)
{
    switch (Version)
    {
    case EGame::PrimeDemo:  return 0xD;
    case EGame::Prime:      return 0x11;
    case EGame::EchoesDemo: return 0x14;
    case EGame::Echoes:     return 0x17;
    case EGame::Corruption: return 0x19;
    case EGame::DKCReturns:    return 0x1B;
    default:          return 0;
    }
}
