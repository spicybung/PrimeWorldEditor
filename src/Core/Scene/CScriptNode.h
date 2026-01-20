#ifndef CSCRIPTNODE_H
#define CSCRIPTNODE_H

#include "Core/Resource/TResPtr.h"
#include "Core/Resource/Script/CScriptTemplate.h"
#include "Core/Scene/CSceneNode.h"

#include <memory>
#include <ranges>
#include <vector>

class CAnimation;
class CCollisionNode;
class CLightParameters;
class CModelNode;
class CResource;
class CScriptAttachNode;
class CScriptExtra;
class CScriptObject;
class CSkeleton;

class CScriptNode : public CSceneNode
{
    CScriptObject *mpInstance;
    CScriptExtra *mpExtra = nullptr;

    TResPtr<CResource> mpDisplayAsset;
    uint32_t mCharIndex = 0;
    uint32_t mAnimIndex = 0;
    CCollisionNode *mpCollisionNode;
    std::vector<CScriptAttachNode*> mAttachments;

    bool mHasValidPosition = false;
    bool mHasVolumePreview = false;
    CModelNode *mpVolumePreviewNode = nullptr;

    std::unique_ptr<CLightParameters> mpLightParameters;

public:
    enum class EGameModeVisibility
    {
        Visible,
        NotVisible,
        Untested,
    };
    EGameModeVisibility mGameModeVisibility{EGameModeVisibility::Untested};

    explicit CScriptNode(CScene *pScene, uint32_t NodeID, CSceneNode *pParent = nullptr, CScriptObject *pObject = nullptr);
    ~CScriptNode() override;

    ENodeType NodeType() const override;
    void PostLoad() override;
    void OnTransformed() override;
    void AddToRenderer(CRenderer* pRenderer, const SViewInfo& rkViewInfo) override;
    void Draw(FRenderOptions Options, int ComponentIndex, ERenderCommand Command, const SViewInfo& rkViewInfo) override;
    void DrawSelection() override;
    void RayAABoxIntersectTest(CRayCollisionTester& rTester, const SViewInfo& rkViewInfo) override;
    SRayIntersection RayNodeIntersectTest(const CRay& rkRay, uint32_t AssetID, const SViewInfo& rkViewInfo) override;
    bool AllowsRotate() const override;
    bool AllowsScale() const override;
    bool IsVisible() const override;
    CColor TintColor(const SViewInfo& rkViewInfo) const override;
    CColor WireframeColor() const override;
    CStructRef GetProperties() const override;
    void PropertyModified(IProperty* pProp) override;

    void LinksModified();
    void UpdatePreviewVolume();
    void GeneratePosition();
    void TestGameModeVisibility();
    CScriptObject* Instance() const;
    CScriptTemplate* Template() const;
    CScriptExtra* Extra() const;
    bool HasPreviewVolume() const;
    CAABox PreviewVolumeAABox() const;
    CVector2f BillboardScale() const;
    CTransform4f BoneTransform(uint32_t BoneID, EAttachType AttachType, bool Absolute) const;

    CModel* ActiveModel() const;
    CAnimSet* ActiveAnimSet() const;
    CSkeleton* ActiveSkeleton() const;
    CAnimation* ActiveAnimation() const;
    CTexture* ActiveBillboard() const;
    bool UsesModel() const;

    auto Attachments() const        { return std::views::all(mAttachments); }
    CResource* DisplayAsset() const { return mpDisplayAsset; }

protected:
    void SetDisplayAsset(CResource *pRes);
    void CalculateTransform(CTransform4f& rOut) const override;
};

#endif // CSCRIPTNODE_H
