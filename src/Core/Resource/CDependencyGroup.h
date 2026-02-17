#ifndef CDEPENDENCYGROUP
#define CDEPENDENCYGROUP

#include "Core/Resource/CResource.h"
#include <algorithm>
#include <ranges>

class CDependencyGroup : public CResource
{
    DECLARE_RESOURCE_TYPE(DependencyGroup)
    std::vector<CAssetID> mDependencies;

public:
    explicit CDependencyGroup(CResourceEntry *pEntry = nullptr) : CResource(pEntry) {}

    void Clear()              { mDependencies.clear(); }
    auto Dependencies() const { return std::views::all(mDependencies); }

    void AddDependency(const CAssetID& rkID)
    {
        if (!HasDependency(rkID))
            mDependencies.push_back(rkID);
    }

    void AddDependency(const CResource* pRes)
    {
        if (pRes != nullptr && !HasDependency(pRes->ID()))
            mDependencies.push_back(pRes->ID());
    }

    void RemoveDependency(const CAssetID& rkID)
    {
        const auto it = std::ranges::find(mDependencies, rkID);
        if (it == mDependencies.cend())
            return;

        mDependencies.erase(it);
    }
    
    bool HasDependency(const CAssetID& rkID) const
    {
        return std::ranges::contains(mDependencies, rkID);
    }

    std::unique_ptr<CDependencyTree> BuildDependencyTree() override
    {
        auto pTree = std::make_unique<CDependencyTree>();

        for (const auto& dep : mDependencies)
            pTree->AddDependency(dep);

        return pTree;
    }
};

#endif // CDEPENDENCYGROUP

