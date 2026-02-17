#ifndef CRENDERER_H
#define CRENDERER_H

#include "Core/OpenGL/CFramebuffer.h"
#include "Core/Render/CRenderBucket.h"
#include "Core/Render/EDepthGroup.h"
#include "Core/Render/ERenderCommand.h"
#include "Core/Render/FRenderOptions.h"

#include <Common/CColor.h>
#include <Common/Math/CAABox.h>

#include <array>

class CAABox;
class CModel;
class IRenderable;
struct SViewInfo;

enum class EBloomMode
{
    NoBloom,
    Bloom,
    BloomMaps,
    FakeBloom
};

/**
 * @todo this rendering subsystem is bad and needs a rewrite
 * there's quite a lot of problems overall, but generally speaking, one of the
 * biggest problems with it is that scene nodes have too much control over how
 * they render, and the renderer doesn't have enough. for example, if a certain
 * render option is set, it should not be up to the node classes to respect that
 * option, the renderer should be able to enforce it. there's a lot of other issues
 * that make the renderer suboptimal and harder to maintain/extend than it should be.
 * this is also a more general issue but graphics stuff needs to be further abstracted
 * so that rendering code isn't directly calling OpenGL functions, ideally it should
 * just have more abstracted code that gets redirected to OpenGL at a lower level so
 * that other graphics backends could be supported in the future without needing to
 * majorly rewrite everything (but I guess that's the point we're at right now anyway).
 * I'm also pretty sure there's been no attempt made whatsoever to reduce the number of
 * shader/texture state changes needed per frame, outside batching world geometry (via
 * CStaticModel), which might be a performance drain.
 *
 * for more complaints about the rendering system implementation, see CSceneNode
 */
class CRenderer
{
    FRenderOptions mOptions{ERenderOption::EnableUVScroll | ERenderOption::EnableBackfaceCull};
    EBloomMode mBloomMode{EBloomMode::NoBloom};
    bool mDrawGrid = true;
    CColor mClearColor;
    uint32_t mContextIndex = UINT32_MAX;
    bool mInitialized = false;
    uint32_t mViewportWidth = 0;
    uint32_t mViewportHeight = 0;
    uint32_t mBloomWidth = 0;
    uint32_t mBloomHeight = 0;
    float mBloomHScale = 0.0f;
    float mBloomVScale = 0.0f;

    CFramebuffer mSceneFramebuffer;
    CFramebuffer mPostProcessFramebuffer;
    std::array<CFramebuffer, 3> mBloomFramebuffers;
    GLint mDefaultFramebuffer = 0;

    CRenderBucket mBackgroundBucket;
    CRenderBucket mMidgroundBucket;
    CRenderBucket mForegroundBucket;
    CRenderBucket mUIBucket;

public:
    // Initialization
    CRenderer();
    ~CRenderer();
    void Init();

    // Accessors
    FRenderOptions RenderOptions() const;
    void ToggleBackfaceCull(bool Enable);
    void ToggleUVAnimation(bool Enable);
    void ToggleGrid(bool Enable);
    void ToggleOccluders(bool Enable);
    void ToggleAlphaDisabled(bool Enable);
    void SetBloom(EBloomMode BloomMode);
    void SetClearColor(const CColor& rkClear);
    void SetViewportSize(uint32_t Width, uint32_t Height);

    // Render
    void RenderBuckets(const SViewInfo& rkViewInfo);
    void RenderBloom();
    void RenderSky(CModel *pSkyboxModel, const SViewInfo& rkViewInfo);
    void AddMesh(IRenderable *pRenderable, int ComponentIndex, const CAABox& rkAABox, bool Transparent, ERenderCommand Command, EDepthGroup DepthGroup = EDepthGroup::Midground);
    void BeginFrame();
    void EndFrame();
    void ClearDepthBuffer();

    // Private
private:
    void InitFramebuffer();
};

#endif // RENDERMANAGER_H
