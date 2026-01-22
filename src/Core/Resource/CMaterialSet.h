#ifndef CMATERIALSET_H
#define CMATERIALSET_H

#include "Core/Resource/CMaterial.h"
#include "Core/Resource/CTexture.h"

#include <algorithm>
#include <memory>
#include <vector>

class CMaterialSet
{
    friend class CMaterialLoader;
    friend class CMaterialCooker;

    std::vector<std::unique_ptr<CMaterial>> mMaterials;

public:
    CMaterialSet() = default;
    ~CMaterialSet() = default;

    std::unique_ptr<CMaterialSet> Clone() const
    {
        auto pOut = std::make_unique<CMaterialSet>();

        pOut->mMaterials.resize(mMaterials.size());
        for (size_t i = 0; i < mMaterials.size(); i++)
            pOut->mMaterials[i] = mMaterials[i]->Clone();

        return pOut;
    }

    size_t NumMaterials() const
    {
        return mMaterials.size();
    }

    CMaterial* MaterialByIndex(size_t Index, bool TryBloom) const
    {
        if (Index >= NumMaterials())
            return nullptr;

        CMaterial* Ret = mMaterials[Index].get();
        if (TryBloom && Ret->GetBloomVersion())
            return Ret->GetBloomVersion();

        return Ret;
    }

    CMaterial* MaterialByName(const TString& rkName) const
    {
        const auto iter = std::ranges::find_if(mMaterials,
                                               [&rkName](const auto& entry) { return entry->Name() == rkName; });

        if (iter == mMaterials.cend())
            return nullptr;

        return iter->get();
    }

    uint32_t MaterialIndexByName(const TString& rkName) const
    {
        for (uint32_t i = 0; i < mMaterials.size(); i++)
        {
            if (mMaterials[i]->Name() == rkName)
                return i;
        }

        return UINT32_MAX;
    }

    void GetUsedTextureIDs(std::set<CAssetID>& rOut) const
    {
        for (const auto& material : mMaterials)
        {
            if (material->IndTexture())
                rOut.insert(material->IndTexture()->ID());

            for (const auto* pass : material->Passes())
            {
                if (const CTexture* tex = pass->Texture())
                    rOut.insert(tex->ID());
            }
        }
    }
};

#endif // CMATERIALSET_H
