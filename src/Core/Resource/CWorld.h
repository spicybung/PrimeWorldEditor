#ifndef CWORLD_H
#define CWORLD_H

#include "Core/Resource/CResource.h"
#include "Core/Resource/CSavedStateID.h"
#include "Core/Resource/Area/CGameArea.h"
#include "Core/Resource/Model/CModel.h"
#include "Core/Resource/StringTable/CStringTable.h"
#include <Common/Math/CTransform4f.h>

#include <memory>
#include <vector>

class CWorld : public CResource
{
    DECLARE_RESOURCE_TYPE(World)
    friend class CWorldLoader;
    friend class CWorldCooker;

    // Instances of CResource pointers are placeholders for unimplemented resource types (eg CMapWorld)
    TString mName;
    TResPtr<CStringTable> mpWorldName;
    TResPtr<CStringTable> mpDarkWorldName;
    TResPtr<CResource>    mpSaveWorld;
    TResPtr<CModel>       mpDefaultSkybox;
    TResPtr<CResource>    mpMapWorld;
    uint32_t mTempleKeyWorldIndex = 0;

    struct STimeAttackData
    {
        bool HasTimeAttack{};
        TString ActNumber;
        float BronzeTime{};
        float SilverTime{};
        float GoldTime{};
        float ShinyGoldTime{};
    };
    STimeAttackData mTimeAttackData{};

    struct SAudioGrp
    {
        CAssetID ResID;
        uint32_t GroupID{};
    };
    std::vector<SAudioGrp> mAudioGrps;

    struct SMemoryRelay
    {
        uint32_t InstanceID{};
        uint32_t TargetID{};
        uint16_t Message{};
        bool Active{};
    };
    std::vector<SMemoryRelay> mMemoryRelays;

    struct SArea
    {
        TString InternalName;
        TResPtr<CStringTable> pAreaName;
        CTransform4f Transform;
        CAABox AetherBox;
        CAssetID AreaResID; // Area resource ID
        CAssetID AreaID; // Internal area ID (same length as an asset ID)
        bool AllowPakDuplicates{};

        std::vector<SMemoryRelay> MemoryRelays; // Only needed for MP1
        std::vector<uint16_t> AttachedAreaIDs;
        std::vector<TString> RelFilenames; // Needs to be removed & generated at cook; temporarily leaving for debugging
        std::vector<uint32_t> RelOffsets;

        struct SDock
        {
            struct SConnectingDock
            {
                uint32_t AreaIndex{};
                uint32_t DockIndex{};
            };
            std::vector<SConnectingDock> ConnectingDocks;
            std::vector<CVector3f> DockCoordinates;
        };
        std::vector<SDock> Docks;

        struct SLayer
        {
            TString LayerName;
            bool Active{};
            CSavedStateID LayerStateID;
        };
        std::vector<SLayer> Layers;
    };
    std::vector<SArea> mAreas;

public:
    explicit CWorld(CResourceEntry *pEntry = nullptr);
    ~CWorld() override;

    std::unique_ptr<CDependencyTree> BuildDependencyTree() override;
    void SetAreaLayerInfo(CGameArea *pArea);
    TString InGameName() const;
    TString AreaInGameName(uint32_t AreaIndex) const;
    uint32_t AreaIndex(const CAssetID& AreaID) const;

    // Serialization
    void Serialize(IArchive& rArc) override;
    friend void Serialize(IArchive& rArc, STimeAttackData& rTimeAttackData);
    friend void Serialize(IArchive& rArc, SMemoryRelay& rMemRelay);
    friend void Serialize(IArchive& rArc, SArea& rArea);
    friend void Serialize(IArchive& rArc, SArea::SDock& rDock);
    friend void Serialize(IArchive& rArc, SArea::SDock::SConnectingDock& rDock);
    friend void Serialize(IArchive& rArc, SArea::SLayer& rLayer);
    friend void Serialize(IArchive& rArc, SAudioGrp& rAudioGrp);

    // Accessors
    const TString& Name() const          { return mName; }
    CStringTable* NameString() const     { return mpWorldName; }
    CStringTable* DarkNameString() const { return mpDarkWorldName; }
    CResource* SaveWorld() const         { return mpSaveWorld; }
    CModel* DefaultSkybox() const        { return mpDefaultSkybox; }
    CResource* MapWorld() const          { return mpMapWorld; }

    size_t NumAreas() const                                                { return mAreas.size(); }
    const CAssetID& AreaResourceID(size_t AreaIndex) const                 { return mAreas[AreaIndex].AreaResID; }
    uint32_t AreaAttachedCount(size_t AreaIndex) const                     { return mAreas[AreaIndex].AttachedAreaIDs.size(); }
    uint32_t AreaAttachedID(size_t AreaIndex, size_t AttachedIndex) const  { return mAreas[AreaIndex].AttachedAreaIDs[AttachedIndex]; }
    const TString& AreaInternalName(size_t AreaIndex) const                { return mAreas[AreaIndex].InternalName; }
    CStringTable* AreaName(size_t AreaIndex) const                         { return mAreas[AreaIndex].pAreaName; }
    bool DoesAreaAllowPakDuplicates(size_t AreaIndex) const                { return mAreas[AreaIndex].AllowPakDuplicates; }

    void SetName(TString rkName)                                     { mName = std::move(rkName); }
    void SetAreaAllowsPakDuplicates(size_t AreaIndex, bool Allow)    { mAreas[AreaIndex].AllowPakDuplicates = Allow; }
};

#endif // CWORLD_H
