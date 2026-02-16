#ifndef CMODELNODE_H
#define CMODELNODE_H

#include "Core/Scene/CSceneNode.h"

class CModel;

class CModelNode : public CSceneNode
{
    TResPtr<CModel> mpModel;
    uint32_t mActiveMatSet = 0;
    bool mWorldModel = false;
    bool mForceAlphaOn = false;
    CColor mTintColor{CColor::White()};
    bool mEnableScanOverlay = false;
    CColor mScanOverlayColor;

public:
    explicit CModelNode(CScene *pScene, uint32_t NodeID, CSceneNode *pParent = nullptr, CModel *pModel = nullptr);
    ~CModelNode() override;

    ENodeType NodeType() const override;
    void PostLoad() override;
    void AddToRenderer(CRenderer* pRenderer, const SViewInfo& rkViewInfo) override;
    void Draw(FRenderOptions Options, int ComponentIndex, ERenderCommand Command, const SViewInfo& rkViewInfo) override;
    void RayAABoxIntersectTest(CRayCollisionTester& Tester, const SViewInfo& rkViewInfo) override;
    SRayIntersection RayNodeIntersectTest(const CRay& Ray, uint32_t AssetID, const SViewInfo& rkViewInfo) override;
    CColor TintColor(const SViewInfo& rkViewInfo) const override;

    // Setters
    void SetModel(CModel *pModel);

    void SetMatSet(uint32_t MatSet)                  { mActiveMatSet = MatSet; }
    void SetWorldModel(bool World)                   { mWorldModel = World; }
    void ForceAlphaEnabled(bool Enable)              { mForceAlphaOn = Enable; }
    void SetTintColor(const CColor& rkTintColor)     { mTintColor = rkTintColor; }
    void ClearTintColor()                            { mTintColor = CColor::White(); }
    void SetScanOverlayEnabled(bool Enable)          { mEnableScanOverlay = Enable; }
    void SetScanOverlayColor(const CColor& rkColor)  { mScanOverlayColor = rkColor; }
    CModel* Model() const                            { return mpModel; }
    uint32_t MatSet() const                          { return mActiveMatSet; }
    bool IsWorldModel() const                        { return mWorldModel; }
    uint32_t FindMeshID() const;
};

#endif // CMODELNODE_H
