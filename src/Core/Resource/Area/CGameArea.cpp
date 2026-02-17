#include "Core/Resource/Area/CGameArea.h"

#include "Core/Resource/Model/SSurface.h"
#include "Core/Resource/Script/CScriptLayer.h"
#include "Core/Resource/Script/CScriptTemplate.h"
#include "Core/Render/CRenderer.h"
#include <Common/Log.h>

#include <algorithm>

CGameArea::CGameArea(CResourceEntry *pEntry)
    : CResource(pEntry)
{
}

CGameArea::~CGameArea()
{
    ClearTerrain();
}

std::unique_ptr<CDependencyTree> CGameArea::BuildDependencyTree()
{
    // Base dependencies
    auto pTree = std::make_unique<CAreaDependencyTree>();

    std::set<CAssetID> MatTextures;
    mpMaterialSet->GetUsedTextureIDs(MatTextures);

    for (const auto& id : MatTextures)
        pTree->AddDependency(id);

    pTree->AddDependency(mPathID);

    if (Game() >= EGame::EchoesDemo)
    {
        pTree->AddDependency(mPortalAreaID);
        pTree->AddDependency(mpPoiToWorldMap);
    }
    
    // Extra deps
    for (const auto& dep : mExtraAreaDeps)
        pTree->AddDependency(dep);

    // Layer dependencies
    std::vector<CAssetID> DummyDeps;

    for (size_t iLayer = 0; iLayer < mScriptLayers.size(); iLayer++)
    {
        const std::vector<CAssetID>& rkExtras = (mExtraLayerDeps.size() > iLayer ? mExtraLayerDeps[iLayer] : DummyDeps);
        pTree->AddScriptLayer(mScriptLayers[iLayer].get(), rkExtras);
    }

    return pTree;
}

void CGameArea::AddWorldModel(std::unique_ptr<CModel>&& pModel)
{
    mVertexCount += pModel->GetVertexCount();
    mTriangleCount += pModel->GetTriangleCount();
    mAABox.ExpandBounds(pModel->AABox());

    mWorldModels.push_back(std::move(pModel));
}

void CGameArea::MergeTerrain()
{
    if (mTerrainMerged)
        return;

    // Nothing really complicated here - iterate through every terrain submesh, add each to a static model
    for (auto& pMdl : mWorldModels)
    {
        const size_t SubmeshCount = pMdl->GetSurfaceCount();

        for (size_t iSurf = 0; iSurf < SubmeshCount; iSurf++)
        {
            SSurface *pSurf = pMdl->GetSurface(iSurf);
            CMaterial *pMat = mpMaterialSet->MaterialByIndex(pSurf->MaterialID, false);

            bool NewMat = true;
            for (auto it = mStaticWorldModels.begin(); it != mStaticWorldModels.end(); ++it)
            {
                if ((*it)->GetMaterial() == pMat)
                {
                    // When we append a new submesh to an existing static model, we bump it to the back of the vector.
                    // This is because mesh ordering actually matters sometimes
                    // (particularly with multi-layered transparent meshes)
                    // so we need to at least try to maintain it.
                    // This is maybe not the most efficient way to do this, but it works.
                    std::rotate(it, it + 1, mStaticWorldModels.end());
                    mStaticWorldModels.back()->AddSurface(pSurf);
                    NewMat = false;
                    break;
                }
            }

            if (NewMat)
            {
                auto pStatic = std::make_unique<CStaticModel>(pMat);
                pStatic->AddSurface(pSurf);
                mStaticWorldModels.push_back(std::move(pStatic));
            }
        }
    }
}

void CGameArea::ClearTerrain()
{
    mWorldModels.clear();
    mStaticWorldModels.clear();
    mpMaterialSet.reset();
    mVertexCount = 0;
    mTriangleCount = 0;
    mTerrainMerged = false;
    mAABox = CAABox::Infinite();
}

void CGameArea::ClearScriptLayers()
{
    mScriptLayers.clear();
}

size_t CGameArea::TotalInstanceCount() const
{
    size_t Num = 0;

    for (const auto& layer : mScriptLayers)
        Num += layer->NumInstances();

    return Num;
}

CScriptObject* CGameArea::InstanceByID(CInstanceID ID)
{
    const auto it = mObjectMap.find(ID);

    if (it != mObjectMap.cend())
        return it->second;

    return nullptr;
}

CInstanceID CGameArea::FindUnusedInstanceID() const
{
    auto ID = CInstanceID((mWorldIndex << 16) | 1);

    while (mObjectMap.contains(ID))
        ID = CInstanceID(ID.Value() + 1);

    return ID;
}

CScriptObject* CGameArea::SpawnInstance(CScriptTemplate *pTemplate,
                                        CScriptLayer *pLayer,
                                        const CVector3f& rkPosition,
                                        const CQuaternion& rkRotation,
                                        const CVector3f& rkScale,
                                        CInstanceID SuggestedID,
                                        uint32_t SuggestedLayerIndex)
{
    // Verify we can fit another instance in this area.
    const size_t NumInstances = TotalInstanceCount();

    if (NumInstances >= 0xFFFF)
    {
        NLog::Error("Unable to spawn a new script instance; too many instances in area ({})", NumInstances);
        return nullptr;
    }

    // Check whether the suggested instance ID is valid
    auto InstanceID = SuggestedID;

    if (InstanceID.IsValid())
    {
        if (!mObjectMap.contains(InstanceID))
            InstanceID.Invalidate();
    }

    // If not valid (or if there's no suggested ID) then determine a new instance ID
    if (InstanceID.IsInvalid())
    {
        // Determine layer index
        const uint32_t LayerIndex = pLayer->AreaIndex();

        if (LayerIndex == UINT32_MAX)
        {
            NLog::Error("Unable to spawn a new script instance; invalid script layer passed in");
            return nullptr;
        }

        // Look for a valid instance ID
        InstanceID = FindUnusedInstanceID();
    }

    // Spawn instance
    auto* pInstance = new CScriptObject(InstanceID, this, pLayer, pTemplate);
    pInstance->EvaluateProperties();
    pInstance->SetPosition(rkPosition);
    pInstance->SetRotation(rkRotation.ToEuler());
    pInstance->SetScale(rkScale);
    pInstance->SetName(pTemplate->Name());
    if (pTemplate->Game() < EGame::EchoesDemo)
        pInstance->SetActive(true);
    pLayer->AddInstance(pInstance, SuggestedLayerIndex);
    mObjectMap[InstanceID] = pInstance;
    return pInstance;
}

void CGameArea::AddInstanceToArea(CScriptObject *pInstance)
{
    // Used for undo after deleting an instance.
    // In the future the script loader should go through SpawnInstance to avoid the need for this function.
    mObjectMap[pInstance->InstanceID()] = pInstance;
}

void CGameArea::DeleteInstance(CScriptObject *pInstance)
{
    pInstance->BreakAllLinks();
    pInstance->Layer()->RemoveInstance(pInstance);
    pInstance->Template()->RemoveObject(pInstance);

    auto it = mObjectMap.find(pInstance->InstanceID());
    if (it != mObjectMap.end())
        mObjectMap.erase(it);

    if (mpPoiToWorldMap && mpPoiToWorldMap->HasPoiMappings(pInstance->InstanceID()))
        mpPoiToWorldMap->RemovePoi(pInstance->InstanceID());

    delete pInstance;
}

void CGameArea::ClearExtraDependencies()
{
    if (!mExtraAreaDeps.empty() || !mExtraLayerDeps.empty())
    {
        mExtraAreaDeps.clear();
        mExtraLayerDeps.clear();
        Entry()->UpdateDependencies();
    }
}
