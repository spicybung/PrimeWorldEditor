#ifndef CMATERIALSET_H
#define CMATERIALSET_H

#include "Core/Resource/CMaterial.h"
#include "Core/Resource/CTexture.h"

#include <memory>
#include <set>
#include <vector>

class CMaterialSet
{
    friend class CMaterialLoader;
    friend class CMaterialCooker;

    std::vector<std::unique_ptr<CMaterial>> mMaterials;

public:
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
