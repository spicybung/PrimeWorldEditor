#ifndef CSCRIPTLAYER_H
#define CSCRIPTLAYER_H

#include "Core/NRangeUtils.h"
#include "Core/Resource/Area/CGameArea.h"
#include "Core/Resource/Script/CScriptObject.h"

#include <algorithm>
#include <cstdint>
#include <vector>

class CScriptLayer
{
    CGameArea *mpArea;
    TString mLayerName{"New Layer"};
    bool mActive = true;
    bool mVisible = true;
    std::vector<CScriptObject*> mInstances;

public:
    explicit CScriptLayer(CGameArea *pArea)
        : mpArea(pArea)
    {
    }

    ~CScriptLayer()
    {
        for (auto* instance : mInstances)
            delete instance;
    }

    // Data Manipulation
    void AddInstance(CScriptObject *pObject, uint32_t Index = UINT32_MAX)
    {
        if (Index != UINT32_MAX && Index < mInstances.size())
        {
            auto it = mInstances.begin();
            std::advance(it, Index);
            mInstances.insert(it, pObject);
        }
        else
        {
            mInstances.push_back(pObject);
        }
    }

    void RemoveInstance(const CScriptObject *pInstance)
    {
        const auto it = std::ranges::find_if(mInstances,
                                             [pInstance](const auto* instance) { return instance == pInstance; });

        if (it == mInstances.cend())
            return;

        mInstances.erase(it);
    }

    void RemoveInstanceByIndex(size_t Index)
    {
        mInstances.erase(mInstances.begin() + Index);
    }

    void RemoveInstanceByID(CInstanceID ID)
    {
        const auto it = std::ranges::find_if(mInstances,
                                             [ID](const auto* instance) { return instance->InstanceID() == ID; });

        if (it == mInstances.cend())
            return;

        mInstances.erase(it);
    }

    void Reserve(size_t Amount)
    {
        mInstances.reserve(Amount);
    }

    // Accessors
    CGameArea* Area() const      { return mpArea; }
    const TString& Name() const  { return mLayerName; }
    bool IsActive() const        { return mActive; }
    bool IsVisible() const       { return mVisible; }
    size_t NumInstances() const  { return mInstances.size(); }
    CScriptObject* InstanceByIndex(size_t Index) const { return mInstances[Index]; }

    CScriptObject* InstanceByID(CInstanceID ID) const
    {
        const auto it = std::ranges::find_if(mInstances,
                                             [ID](const auto* instance) { return instance->InstanceID() == ID; });

        if (it == mInstances.cbegin())
            return nullptr;

        return *it;
    }

    void SetName(TString rkName)   { mLayerName = std::move(rkName); }
    void SetActive(bool Active)    { mActive = Active; }
    void SetVisible(bool Visible)  { mVisible = Visible; }

    uint32_t AreaIndex() const
    {
        for (const auto [idx, layer] : Utils::enumerate(mpArea->ScriptLayers()))
        {
            if (layer == this)
                return uint32_t(idx);
        }

        return UINT32_MAX;
    }

    // Operators
    CScriptObject* operator[](size_t Index) { return InstanceByIndex(Index); }
    const CScriptObject* operator[](size_t Index) const { return InstanceByIndex(Index); }
};

#endif // CSCRIPTLAYER_H
