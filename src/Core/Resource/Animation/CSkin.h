#ifndef CSKIN_H
#define CSKIN_H

#include "Core/Resource/CResource.h"
#include "Core/Resource/Model/CVertex.h"
#include <vector>

struct SVertexWeights
{
    TBoneIndices Indices{};
    TBoneWeights Weights{};
};

class CSkin : public CResource
{
    DECLARE_RESOURCE_TYPE(Skin)
    friend class CSkinLoader;

    struct SVertGroup
    {
        SVertexWeights Weights;
        uint32_t NumVertices{};
    };
    std::vector<SVertGroup> mVertGroups;

public:
    explicit CSkin(CResourceEntry *pEntry = nullptr) : CResource(pEntry) {}

    const SVertexWeights& WeightsForVertex(uint32_t VertIdx) const
    {
        // Null weights bind everything to the root bone in case there is no matching vertex group
        static constexpr SVertexWeights skNullWeights{
            {3, 0, 0, 0},
            {1.f, 0.f, 0.f, 0.f},
        };

        uint32_t Index = 0;

        for (const auto& group : mVertGroups)
        {
            if (VertIdx < Index + group.NumVertices)
                return group.Weights;

            Index += group.NumVertices;
        }

        return skNullWeights;
    }
};

#endif // CSKIN_H
