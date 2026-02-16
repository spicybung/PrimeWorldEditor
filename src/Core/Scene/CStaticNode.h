#ifndef CSTATICNODE_H
#define CSTATICNODE_H

#include <Core/Scene/CSceneNode.h>

class CStaticModel;

class CStaticNode : public CSceneNode
{
    CStaticModel *mpModel;

public:
    CStaticNode(CScene *pScene, uint32_t NodeID, CSceneNode *pParent = nullptr, CStaticModel *pModel = nullptr);
    ~CStaticNode() override;

    ENodeType NodeType() const override;
    void PostLoad() override;
    void AddToRenderer(CRenderer* pRenderer, const SViewInfo& rkViewInfo) override;
    void Draw(FRenderOptions Options, int ComponentIndex, ERenderCommand Command, const SViewInfo& rkViewInfo) override;
    void RayAABoxIntersectTest(CRayCollisionTester& rTester, const SViewInfo& rkViewInfo) override;
    SRayIntersection RayNodeIntersectTest(const CRay& rkRay, uint32_t AssetID, const SViewInfo& rkViewInfo) override;
};

#endif // CSTATICNODE_H
