#include "Editor/CollisionEditor/CCollisionEditorViewport.h"

#include <Core/Render/CRenderer.h>
#include <Core/Scene/CCollisionNode.h>

/** Constructor */
CCollisionEditorViewport::CCollisionEditorViewport(QWidget* pParent)
    : CBasicViewport(pParent)
    , mpRenderer{std::make_unique<CRenderer>()}
{
    const auto pixelRatio = devicePixelRatio();
    mpRenderer->SetViewportSize(width() * pixelRatio, height() * pixelRatio);
    mpRenderer->SetClearColor(CColor(0.3f, 0.3f, 0.3f));
    mpRenderer->ToggleGrid(true);

    mViewInfo.ShowFlags = EShowFlag::WorldCollision | EShowFlag::ObjectCollision;
    mViewInfo.pRenderer = mpRenderer.get();
    mViewInfo.pScene = nullptr;
    mViewInfo.GameMode = false;
    mViewInfo.CollisionSettings.DrawBoundingHierarchy = false;
    mViewInfo.CollisionSettings.BoundingHierarchyRenderDepth = 0;
}

CCollisionEditorViewport::~CCollisionEditorViewport() = default;

/** CBasicViewport interface */
void CCollisionEditorViewport::Paint()
{
    mpRenderer->BeginFrame();
    mCamera.LoadMatrices();

    if (mGridEnabled)
        mGrid.AddToRenderer(mpRenderer.get(), mViewInfo);

    if (mpCollisionNode)
    {
        mpCollisionNode->AddToRenderer(mpRenderer.get(), mViewInfo);
    }

    mpRenderer->RenderBuckets(mViewInfo);
    mpRenderer->EndFrame();
}

void CCollisionEditorViewport::OnResize()
{
    const auto pixelRatio = devicePixelRatio();
    mpRenderer->SetViewportSize(width() * pixelRatio, height() * pixelRatio);
}
