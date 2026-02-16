#ifndef CROOTNODE_H
#define CROOTNODE_H

#include "Core/Scene/CSceneNode.h"

// CRootNode's main purpose is to manage groups of other nodes as its children.
class CRootNode : public CSceneNode
{
public:
    explicit CRootNode(CScene *pScene, uint32_t NodeID, CSceneNode *pParent = nullptr)
        : CSceneNode(pScene, NodeID, pParent) {}

    ENodeType NodeType() const override
    {
        return ENodeType::Root;
    }

    void RayAABoxIntersectTest(CRayCollisionTester&, const SViewInfo&) override {}

    SRayIntersection RayNodeIntersectTest(const CRay&, uint32_t, const SViewInfo&) override
    {
        return {};
    }
};

#endif // CROOTNODE_H
