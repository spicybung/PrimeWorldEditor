#include "Core/Scene/CCharacterNode.h"

#include "Core/SRayIntersection.h"
#include "Core/Render/CGraphics.h"
#include "Core/Render/CRenderer.h"

CCharacterNode::CCharacterNode(CScene *pScene, uint32_t NodeID, CAnimSet *pChar /*= 0*/, CSceneNode *pParent /*= 0*/)
    : CSceneNode(pScene, NodeID, pParent)
    , mAnimated(true)
    , mAnimTime(0.f)
{
    SetCharSet(pChar);
}

CCharacterNode::~CCharacterNode() = default;

ENodeType CCharacterNode::NodeType() const
{
    return ENodeType::Character;
}

void CCharacterNode::PostLoad()
{
    if (mpCharacter == nullptr)
        return;

    for (const auto& character : mpCharacter->Characters())
        character.pModel->BufferGL();
}

void CCharacterNode::AddToRenderer(CRenderer *pRenderer, const SViewInfo& rkViewInfo)
{
    // todo: frustum check. Currently don't have a means of pulling the AABox for the
    // current animation so this isn't in yet.
    if (!mpCharacter)
        return;

    UpdateTransformData();

    CModel *pModel = mpCharacter->Character(mActiveCharSet)->pModel;
    CSkeleton *pSkel = mpCharacter->Character(mActiveCharSet)->pSkeleton;

    if (pModel && rkViewInfo.ShowFlags.HasFlag(EShowFlag::ObjectGeometry))
        AddModelToRenderer(pRenderer, pModel, 0);

    if (pSkel)
    {
        if (rkViewInfo.ShowFlags.HasFlag(EShowFlag::Skeletons))
            pRenderer->AddMesh(this, 0, AABox(), false, ERenderCommand::DrawMesh, EDepthGroup::Foreground);
    }
}

void CCharacterNode::Draw(FRenderOptions Options, int ComponentIndex, ERenderCommand Command, const SViewInfo& rkViewInfo)
{
    if (Command == ERenderCommand::DrawSelection)
    {
        CSceneNode::DrawSelection();
        return;
    }

    CSkeleton *pSkel = mpCharacter->Character(mActiveCharSet)->pSkeleton;

    // Draw skeleton
    if (ComponentIndex == 0)
    {
        LoadModelMatrix();
        pSkel->Draw(Options, &mTransformData);
    }
    else // Draw mesh
    {
        // Set lighting
        CGraphics::SetDefaultLighting();
        CGraphics::UpdateLightBlock();
        CGraphics::sVertexBlock.COLOR0_Amb = CGraphics::skDefaultAmbientColor;
        CGraphics::sVertexBlock.COLOR0_Mat = CColor::TransparentWhite();
        CGraphics::sPixelBlock.LightmapMultiplier = 1.f;
        CGraphics::sPixelBlock.SetAllTevColors(CColor::White());
        CGraphics::sPixelBlock.TintColor = TintColor(rkViewInfo);
        LoadModelMatrix();

        // Draw surface OR draw entire model
        if (mAnimated)
            CGraphics::LoadBoneTransforms(mTransformData);
        else
            CGraphics::LoadIdentityBoneTransforms();

        CModel *pModel = mpCharacter->Character(mActiveCharSet)->pModel;
        DrawModelParts(pModel, Options, 0, Command);
    }
}

SRayIntersection CCharacterNode::RayNodeIntersectTest(const CRay& rkRay, uint32_t /*AssetID*/, const SViewInfo& rkViewInfo)
{
    // Check for bone under ray. Doesn't check for model intersections atm
    if (mpCharacter && rkViewInfo.ShowFlags.HasFlag(EShowFlag::Skeletons))
    {
        CSkeleton *pSkel = mpCharacter->Character(mActiveCharSet)->pSkeleton;

        if (pSkel)
        {
            UpdateTransformData();
            const auto [index, distance] = pSkel->RayIntersect(rkRay, mTransformData);

            if (index != -1)
            {
                return {
                    .Hit = true,
                    .Distance = distance,
                    .HitPoint = rkRay.PointOnRay(distance),
                    .pNode = this,
                    .ComponentIndex = uint32_t(index),
                };
            }
        }
    }

    return {};
}

CVector3f CCharacterNode::BonePosition(uint32_t BoneID)
{
    UpdateTransformData();
    const CSkeleton* pSkel = mpCharacter ? mpCharacter->Character(mActiveCharSet)->pSkeleton : nullptr;
    const CBone* pBone = pSkel ? pSkel->BoneByID(BoneID) : nullptr;

    CVector3f Out = AbsolutePosition();
    if (pBone)
        Out += pBone->TransformedPosition(mTransformData);
    return Out;
}

void CCharacterNode::SetCharSet(CAnimSet *pChar)
{
    mpCharacter = pChar;
    SetActiveChar(0);
    SetActiveAnim(0);
    ConditionalSetDirty();

    if (!mpCharacter)
        mLocalAABox = CAABox::One();
}

void CCharacterNode::SetActiveChar(uint32_t CharIndex)
{
    mActiveCharSet = CharIndex;
    ConditionalSetDirty();

    if (mpCharacter)
    {
        const CModel* pModel = mpCharacter->Character(CharIndex)->pModel;
        mTransformData.ResizeToSkeleton(mpCharacter->Character(CharIndex)->pSkeleton);
        mLocalAABox = pModel ? pModel->AABox() : CAABox::Zero();
        MarkTransformChanged();
    }
}

void CCharacterNode::SetActiveAnim(uint32_t AnimIndex)
{
    mActiveAnim = AnimIndex;
    ConditionalSetDirty();
}

// ************ PROTECTED ************
void CCharacterNode::UpdateTransformData()
{
    if (mTransformDataDirty)
    {
        if (CSkeleton* pSkel = mpCharacter->Character(mActiveCharSet)->pSkeleton)
            pSkel->UpdateTransform(mTransformData, CurrentAnim(), mAnimTime, false);

        mTransformDataDirty = false;
    }
}
