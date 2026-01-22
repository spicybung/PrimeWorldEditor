#include "CPoiToWorld.h"

#include <algorithm>

CPoiToWorld::CPoiToWorld(CResourceEntry *pEntry)
    : CResource(pEntry)
{
}

CPoiToWorld::~CPoiToWorld() = default;

void CPoiToWorld::AddPoi(CInstanceID PoiID)
{
    // Check if this POI already exists
    const auto it = mPoiLookupMap.find(PoiID);
    if (it != mPoiLookupMap.end())
        return;

    auto pMap = std::make_unique<SPoiMap>();
    pMap->PoiID = PoiID;

    auto* ptr = mMaps.emplace_back(std::move(pMap)).get();
    mPoiLookupMap.insert_or_assign(PoiID, ptr);
}

void CPoiToWorld::AddPoiMeshMap(CInstanceID PoiID, uint32_t ModelID)
{
    // Make sure the POI exists; the add function won't do anything if it does
    AddPoi(PoiID);
    SPoiMap *pMap = mPoiLookupMap[PoiID];

    // Check whether this model ID is already mapped to this POI
    const auto AlreadyMapped = std::ranges::any_of(pMap->ModelIDs, [&](const auto& id) { return id == ModelID; });
    if (AlreadyMapped)
        return;

    // We didn't return, so this is a new mapping
    pMap->ModelIDs.push_back(ModelID);
}

void CPoiToWorld::RemovePoi(CInstanceID PoiID)
{
    const auto it = std::ranges::find_if(mMaps, [&](const auto& entry) { return entry->PoiID == PoiID; });
    if (it == mMaps.end())
        return;

    mMaps.erase(it);
    mPoiLookupMap.erase(PoiID);
}

void CPoiToWorld::RemovePoiMeshMap(CInstanceID PoiID, uint32_t ModelID)
{
    const auto MapIt = mPoiLookupMap.find(PoiID);
    if (MapIt == mPoiLookupMap.end())
        return;

    SPoiMap* pMap = MapIt->second;
    const auto ListIt = std::ranges::find(pMap->ModelIDs, ModelID);
    if (ListIt == pMap->ModelIDs.end())
        return;

    pMap->ModelIDs.erase(ListIt);
}
