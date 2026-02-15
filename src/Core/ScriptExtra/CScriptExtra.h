#ifndef CSCRIPTEXTRA_H
#define CSCRIPTEXTRA_H

#include "Core/Scene/CSceneNode.h"

class CScriptNode;
class CScriptObject;

/**
 * CScriptExtra is a class that allows for additional coded behavior on any given
 * script object type. Subclass IScriptExtra, add the new class to CScriptExtra.cpp,
 * and reimplement whatever functions are needed to create the desired behavior. Note
 * that in addition to the functions here you can also reimplement IRenderable functions
 * (to render additional geometry) and CSceneNode functions (primarily for raycast
 * intersections).
 *
 * @todo I think I'd kinda rather CScriptExtra just inherited CScriptObject instead of
 * being a separate node type on top of it. I remember there were reasons I didn't do it
 * that way to begin with but I can't remember what they were off the top of my head.
 */

class CScriptExtra : public CSceneNode
{
protected:
    CScriptNode *mpScriptNode;
    CScriptObject *mpInstance;
    EGame mGame;

public:
    explicit CScriptExtra(CScriptObject* pInstance, CScene* pScene, CScriptNode* pParent = nullptr);
    ~CScriptExtra() override;

    CScriptObject* Instance() const  { return mpInstance; }
    EGame Game() const               { return mGame; }

    // Default implementations for CSceneNode
    ENodeType NodeType() const override { return ENodeType::ScriptExtra; }
    void RayAABoxIntersectTest(CRayCollisionTester& /*rTester*/, const SViewInfo& /*rkViewInfo*/) override {}
    SRayIntersection RayNodeIntersectTest(const CRay& /*rkRay*/, uint32_t /*AssetID*/, const SViewInfo& /*rkViewInfo*/) override;
    CColor WireframeColor() const override { return mpParent->WireframeColor(); }

    // Virtual CScriptExtra functions
    virtual void InstanceTransformed() {}
    void PropertyModified(IProperty* /*pProperty*/) override {}
    virtual void DisplayAssetChanged(CResource* /*pNewDisplayAsset*/) {}
    virtual void LinksModified() {}
    virtual bool ShouldDrawNormalAssets() const { return true; }
    virtual bool ShouldDrawVolume() const { return true; }
    virtual CColor TevColor() const { return CColor::White(); }
    virtual void ModifyTintColor(CColor& /*Color*/) {}

    // Create Script Extra
    static CScriptExtra* CreateExtra(CScriptNode *pNode);
};

#endif // CSCRIPTEXTRA_H
