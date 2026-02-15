#ifndef CRENDERBUCKET_H
#define CRENDERBUCKET_H

#include <cstdint>
#include <vector>

class CCamera;
struct SRenderablePtr;
struct SViewInfo;

class CRenderBucket
{
    class CSubBucket
    {
        std::vector<SRenderablePtr> mRenderables;

    public:
        CSubBucket();
        ~CSubBucket();

        void Add(const SRenderablePtr &rkPtr);
        void Sort(const CCamera *pkCamera, bool DebugVisualization);
        void Clear();
        void Draw(const SViewInfo& rkViewInfo);
    };

    CSubBucket mOpaqueSubBucket;
    CSubBucket mTransparentSubBucket;
    bool mEnableDepthSortDebugVisualization = false;

public:
    CRenderBucket();
    ~CRenderBucket();

    void Add(const SRenderablePtr& rkPtr, bool Transparent);
    void Clear();
    void Draw(const SViewInfo& rkViewInfo);
};

#endif // CRENDERBUCKET_H
