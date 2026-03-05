#include "Core/Resource/Cooker/CPoiToWorldCooker.h"

#include "Core/Resource/CPoiToWorld.h"
#include <algorithm>
#include <compare>
#include <vector>

bool CPoiToWorldCooker::CookEGMC(const CPoiToWorld* pPoiToWorld, IOutputStream& rOut)
{
    // Create mappings list
    struct SPoiMapping
    {
        uint32_t MeshID{};
        CInstanceID PoiID;

        constexpr auto operator<=>(const SPoiMapping&) const noexcept = default;
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

    // Echoes typically sorts in ascending order
    std::ranges::sort(Mappings);

    // Write EGMC
    rOut.WriteU32(static_cast<uint32_t>(Mappings.size()));

    // TODO: See CPoiToWorldLoader. We currently mask out the area index from the instance ID
    //       with & 0x03FFFFFF, which results in incorrect POI IDs being cooked.
    for (const auto& mapping : Mappings)
    {
        rOut.WriteU32(mapping.MeshID);
        rOut.WriteU32(mapping.PoiID.Value());
    }

    return true;
}
