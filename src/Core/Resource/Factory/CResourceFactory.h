#ifndef CRESOURCEFACTORY
#define CRESOURCEFACTORY

#include "Core/Resource/Factory/CAnimationLoader.h"
#include "Core/Resource/Factory/CAnimEventLoader.h"
#include "Core/Resource/Factory/CAnimSetLoader.h"
#include "Core/Resource/Factory/CAreaLoader.h"
#include "Core/Resource/Factory/CAudioGroupLoader.h"
#include "Core/Resource/Factory/CCollisionLoader.h"
#include "Core/Resource/Factory/CDependencyGroupLoader.h"
#include "Core/Resource/Factory/CFontLoader.h"
#include "Core/Resource/Factory/CMaterialLoader.h"
#include "Core/Resource/Factory/CModelLoader.h"
#include "Core/Resource/Factory/CPoiToWorldLoader.h"
#include "Core/Resource/Factory/CScanLoader.h"
#include "Core/Resource/Factory/CScriptLoader.h"
#include "Core/Resource/Factory/CSkeletonLoader.h"
#include "Core/Resource/Factory/CSkinLoader.h"
#include "Core/Resource/Factory/CStringLoader.h"
#include "Core/Resource/Factory/CTextureDecoder.h"
#include "Core/Resource/Factory/CUnsupportedFormatLoader.h"
#include "Core/Resource/Factory/CUnsupportedParticleLoader.h"
#include "Core/Resource/Factory/CWorldLoader.h"

#include "Core/Resource/CAudioGroup.h"
#include "Core/Resource/CAudioMacro.h"
#include "Core/Resource/CAudioLookupTable.h"
#include "Core/Resource/CDependencyGroup.h"
#include "Core/Resource/CMapArea.h"
#include "Core/Resource/CStringList.h"
#include "Core/Resource/Resources.h"
#include "Core/Tweaks/CTweakData.h"
#include "Core/Tweaks/CTweakLoader.h"

// Static helper class to allow spawning resources based on an EResType
class CResourceFactory
{
public:
    CResourceFactory() = delete;

    static std::unique_ptr<CResource> CreateResource(CResourceEntry *pEntry)
    {
        switch (pEntry->ResourceType())
        {
        case EResourceType::Animation:            return std::make_unique<CAnimation>(pEntry);
        case EResourceType::AnimEventData:        return std::make_unique<CAnimEventData>(pEntry);
        case EResourceType::AnimSet:              return std::make_unique<CAnimSet>(pEntry);
        case EResourceType::Area:                 return std::make_unique<CGameArea>(pEntry);
        case EResourceType::AudioMacro:           return std::make_unique<CAudioMacro>(pEntry);
        case EResourceType::AudioGroup:           return std::make_unique<CAudioGroup>(pEntry);
        case EResourceType::AudioLookupTable:     return std::make_unique<CAudioLookupTable>(pEntry);
        case EResourceType::Character:            return std::make_unique<CAnimSet>(pEntry);
        case EResourceType::DynamicCollision:     return std::make_unique<CCollisionMeshGroup>(pEntry);
        case EResourceType::DependencyGroup:      return std::make_unique<CDependencyGroup>(pEntry);
        case EResourceType::Font:                 return std::make_unique<CFont>(pEntry);
        case EResourceType::MapArea:              return std::make_unique<CMapArea>(pEntry);
        case EResourceType::Model:                return std::make_unique<CModel>(pEntry);
        case EResourceType::Scan:                 return std::make_unique<CScan>(pEntry);
        case EResourceType::Skeleton:             return std::make_unique<CSkeleton>(pEntry);
        case EResourceType::Skin:                 return std::make_unique<CSkin>(pEntry);
        case EResourceType::SourceAnimData:       return std::make_unique<CSourceAnimData>(pEntry);
        case EResourceType::StaticGeometryMap:    return std::make_unique<CPoiToWorld>(pEntry);
        case EResourceType::StringList:           return std::make_unique<CStringList>(pEntry);
        case EResourceType::StringTable:          return std::make_unique<CStringTable>(pEntry);
        case EResourceType::Texture:              return std::make_unique<CTexture>(pEntry);
        case EResourceType::World:                return std::make_unique<CWorld>(pEntry);
        default:                                  return nullptr; // should it return a CResource instead?
        }
    }

    static std::unique_ptr<CResource> LoadCookedResource(CResourceEntry *pEntry, IInputStream& rInput)
    {
        // Warning: It is the caller's responsibility to check if the required resource is already in memory before calling this function.
        if (!rInput.IsValid())
            return nullptr;

        switch (pEntry->ResourceType())
        {
        case EResourceType::Animation:            return CAnimationLoader::LoadANIM(rInput, pEntry);
        case EResourceType::AnimEventData:        return CAnimEventLoader::LoadEVNT(rInput, pEntry);
        case EResourceType::AnimSet:              return CAnimSetLoader::LoadANCS(rInput, pEntry);
        case EResourceType::Area:                 return CAreaLoader::LoadMREA(rInput, pEntry);
        case EResourceType::AudioMacro:           return CUnsupportedFormatLoader::LoadCAUD(rInput, pEntry);
        case EResourceType::AudioGroup:           return CAudioGroupLoader::LoadAGSC(rInput, pEntry);
        case EResourceType::AudioLookupTable:     return CAudioGroupLoader::LoadATBL(rInput, pEntry);
        case EResourceType::BinaryData:           return CUnsupportedFormatLoader::LoadDUMB(rInput, pEntry);
        case EResourceType::Character:            return CAnimSetLoader::LoadCHAR(rInput, pEntry);
        case EResourceType::DependencyGroup:      return CDependencyGroupLoader::LoadDGRP(rInput, pEntry);
        case EResourceType::DynamicCollision:     return CCollisionLoader::LoadDCLN(rInput, pEntry);
        case EResourceType::Font:                 return CFontLoader::LoadFONT(rInput, pEntry);
        case EResourceType::GuiFrame:             return CUnsupportedFormatLoader::LoadFRME(rInput, pEntry);
        case EResourceType::HintSystem:           return CUnsupportedFormatLoader::LoadHINT(rInput, pEntry);
        case EResourceType::MapArea:              return CUnsupportedFormatLoader::LoadMAPA(rInput, pEntry);
        case EResourceType::MapWorld:             return CUnsupportedFormatLoader::LoadMAPW(rInput, pEntry);
        case EResourceType::MapUniverse:          return CUnsupportedFormatLoader::LoadMAPU(rInput, pEntry);
        case EResourceType::Midi:                 return CUnsupportedFormatLoader::LoadCSNG(rInput, pEntry);
        case EResourceType::Model:                return CModelLoader::LoadCMDL(rInput, pEntry);
        case EResourceType::RuleSet:              return CUnsupportedFormatLoader::LoadRULE(rInput, pEntry);
        case EResourceType::Scan:                 return CScanLoader::LoadSCAN(rInput, pEntry);
        case EResourceType::Skeleton:             return CSkeletonLoader::LoadCINF(rInput, pEntry);
        case EResourceType::Skin:                 return CSkinLoader::LoadCSKR(rInput, pEntry);
        case EResourceType::SourceAnimData:       return CAnimSetLoader::LoadSAND(rInput, pEntry);
        case EResourceType::StateMachine2:        return CUnsupportedFormatLoader::LoadFSM2(rInput, pEntry);
        case EResourceType::StaticGeometryMap:    return CPoiToWorldLoader::LoadEGMC(rInput, pEntry);
        case EResourceType::StringList:           return CAudioGroupLoader::LoadSTLC(rInput, pEntry);
        case EResourceType::StringTable:          return CStringLoader::LoadSTRG(rInput, pEntry);
        case EResourceType::Texture:              return CTextureDecoder::LoadTXTR(rInput, pEntry);
        case EResourceType::Tweaks:               return CTweakLoader::LoadCTWK(rInput, pEntry);
        case EResourceType::World:                return CWorldLoader::LoadMLVL(rInput, pEntry);

        case EResourceType::StateMachine:
            // AFSM currently unsupported
            if (pEntry->Game() <= EGame::Echoes)
                return std::make_unique<CDependencyGroup>(pEntry);
            if (pEntry->Game() <= EGame::Corruption)
                return CUnsupportedFormatLoader::LoadFSM2(rInput, pEntry);
            if (pEntry->Game() == EGame::DKCReturns)
                return CUnsupportedFormatLoader::LoadFSMC(rInput, pEntry);
            return nullptr;

        case EResourceType::BurstFireData:
        case EResourceType::Particle:
        case EResourceType::ParticleElectric:
        case EResourceType::ParticleSorted:
        case EResourceType::ParticleSpawn:
        case EResourceType::ParticleSwoosh:
        case EResourceType::ParticleDecal:
        case EResourceType::ParticleWeapon:
        case EResourceType::ParticleCollisionResponse:
        case EResourceType::ParticleTransform:
        case EResourceType::UserEvaluatorData:
            return CUnsupportedParticleLoader::LoadParticle(rInput, pEntry);

        default:
            return std::make_unique<CResource>(pEntry);
        }
    }
};

#endif // CRESOURCEFACTORY

