#include "Core/Resource/Cooker/CPoiToWorldCooker.h"

#include "Core/Resource/CPoiToWorld.h"
#include <vector>

bool CPoiToWorldCooker::CookEGMC(const CPoiToWorld* pPoiToWorld, IOutputStream& rOut)
{
    // Create mappings list
    struct SPoiMapping
    {
        uint32_t MeshID{};
        CInstanceID PoiID;
    };
    std::vector<SPoiMapping> Mappings;

    for (size_t iPoi = 0; iPoi < pPoiToWorld->NumMappedPOIs(); iPoi++)
    {
        const CPoiToWorld::SPoiMap* pkMap = pPoiToWorld->MapByIndex(iPoi);

        for (const auto meshID : pkMap->ModelIDs)
        {
            Mappings.emplace_back(meshID, pkMap->PoiID);
        }
    }

    // Write EGMC
    rOut.WriteU32(static_cast<uint32_t>(Mappings.size()));

    for (const auto& mapping : Mappings)
    {
        rOut.WriteU32(mapping.MeshID);
        rOut.WriteU32(mapping.PoiID.Value());
    }

    return true;
}
