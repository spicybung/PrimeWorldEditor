#ifndef CRESOURCECOOKER_H
#define CRESOURCECOOKER_H

#include "Core/GameProject/CResourceEntry.h"
#include "Core/Resource/Cooker/CAreaCooker.h"
#include "Core/Resource/Cooker/CModelCooker.h"
#include "Core/Resource/Cooker/CPoiToWorldCooker.h"
#include "Core/Resource/Cooker/CScanCooker.h"
#include "Core/Resource/Cooker/CStringCooker.h"
#include "Core/Resource/Cooker/CWorldCooker.h"
#include "Core/Tweaks/CTweakCooker.h"

#include <Common/Log.h>

class CResourceCooker
{
public:
    CResourceCooker() = delete;
    static bool CookResource(CResourceEntry *pEntry, IOutputStream& rOutput)
    {
        CResource *pRes = pEntry->Load();
        ASSERT(pRes != nullptr);

        switch (pEntry->ResourceType())
        {
        case EResourceType::Area:                 return CAreaCooker::CookMREA((CGameArea*) pRes, rOutput);
        case EResourceType::Model:                return CModelCooker::CookCMDL((CModel*) pRes, rOutput);
        case EResourceType::Scan:                 return CScanCooker::CookSCAN((CScan*) pRes, rOutput);
        case EResourceType::StaticGeometryMap:    return CPoiToWorldCooker::CookEGMC((CPoiToWorld*) pRes, rOutput);
        case EResourceType::StringTable:          return CStringCooker::CookSTRG((CStringTable*) pRes, rOutput);
        case EResourceType::Tweaks:               return CTweakCooker::CookCTWK((CTweakData*) pRes, rOutput);
        case EResourceType::World:                return CWorldCooker::CookMLVL((CWorld*) pRes, rOutput);

        default:
            NLog::Warn("Failed to cook {} asset; this resource type is not supported for cooking", pEntry->CookedExtension().ToString());
            return false;
        }
    }
};

#endif // CRESOURCECOOKER_H
