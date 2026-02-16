#ifndef CLIGHTNODE_H
#define CLIGHTNODE_H

#include "Core/Scene/CSceneNode.h"

class CLight;

class CLightNode : public CSceneNode
{
    CLight *mpLight;

public:
    CLightNode(CScene *pScene, uint32_t NodeID, CSceneNode *pParent = nullptr, CLight *Light = nullptr);
    ~CLightNode() override;

    ENodeType NodeType() const override;
    void AddToRenderer(CRenderer* pRenderer, const SViewInfo& ViewInfo) override;
    void Draw(FRenderOptions Options, int ComponentIndex, ERenderCommand Command, const SViewInfo& ViewInfo) override;
    void RayAABoxIntersectTest(CRayCollisionTester& Tester, const SViewInfo& ViewInfo) override;
    SRayIntersection RayNodeIntersectTest(const CRay& Ray, uint32_t AssetID, const SViewInfo& ViewInfo) override;
    CStructRef GetProperties() const override;
    void PropertyModified(IProperty* pProperty) override;
    bool AllowsRotate() const override { return false; }
    CLight* Light() { return mpLight; }
    const CLight* Light() const { return mpLight; }
    CVector2f BillboardScale() const;

protected:
    void CalculateTransform(CTransform4f& rOut) const override;
};

#endif // CLIGHTNODE_H
