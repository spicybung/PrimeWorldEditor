#ifndef CARRAYPROPERTY_H
#define CARRAYPROPERTY_H

#include "Core/Resource/Script/Property/IProperty.h"

#include <Common/Math/MathUtil.h>

#include <algorithm>
#include <memory>
#include <vector>

struct SScriptArray
{
    int Count = 0;
    std::vector<char> Array;

    SScriptArray() = default;
    bool operator==(const SScriptArray&) const = default;
};

/** You probably shouldn't use this on intrinsic classes; script only */
/** @todo proper support of default values for arrays (this would be used for prefabs) */
class CArrayProperty : public TTypedProperty<uint32_t, EPropertyType::Array>
{
    friend class IProperty;

    /** This class inherits from TTypedPropertyNew<int> in order to expose the array
     *  count value (the first member of SScriptArray). Outside users can edit this
     *  value and we respond by updating the allocated space, handling item destruction
     *  and construction, etc.
     */
    std::unique_ptr<IProperty> mpItemArchetype;

    /** Internal functions */
    SScriptArray& _GetInternalArray(void* pData)
    {
        return *static_cast<SScriptArray*>(RawValuePtr(pData));
    }
    const SScriptArray& _GetInternalArray(const void* pData) const
    {
        return *static_cast<const SScriptArray*>(RawValuePtr(pData));
    }

    uint32_t _InternalArrayCount(void* pPropertyData)
    {
        const auto& rArray = _GetInternalArray(pPropertyData).Array;
        return rArray.size() / ItemSize();
    }
    uint32_t _InternalArrayCount(const void* pPropertyData) const
    {
        const auto& rArray = _GetInternalArray(pPropertyData).Array;
        return rArray.size() / ItemSize();
    }

protected:
    explicit CArrayProperty(EGame Game)
        : TTypedProperty(Game)
    {}

public:
    uint32_t DataSize() const override
    {
        return sizeof(SScriptArray);
    }

    uint32_t DataAlignment() const override
    {
        return alignof(SScriptArray);
    }

    void Construct(void* pData) override
    {
        new(ValuePtr(pData)) SScriptArray;
    }

    void Destruct(void* pData) override
    {
        RevertToDefault(pData);
        _GetInternalArray(pData).~SScriptArray();
    }

    bool MatchesDefault(const void* pData) const override
    {
        return ArrayCount(pData) == 0;
    }

    void RevertToDefault(void* pData) override
    {
        Resize(pData, 0);
        ValueRef(pData) = 0;
    }

    bool CanHaveDefault() const override
    {
        return true;
    }

    bool IsPointerType() const override
    {
        return true;
    }

    void* GetChildDataPointer(void* pPropertyData) override
    {
        return _GetInternalArray(pPropertyData).Array.data();
    }

    const void* GetChildDataPointer(const void* pPropertyData) const override
    {
        return _GetInternalArray(pPropertyData).Array.data();
    }

    void PropertyValueChanged(void* pPropertyData) override
    {
        SScriptArray& rArray = _GetInternalArray(pPropertyData);
        rArray.Count = std::max(rArray.Count, 0);
        Resize(pPropertyData, rArray.Count);
    }

    void Serialize(IArchive& rArc) override
    {
        TTypedProperty::Serialize(rArc);
        rArc << SerialParameter("ItemArchetype", mpItemArchetype);
    }

    void SerializeValue(void* pData, IArchive& Arc) override
    {
        uint32_t Count = ArrayCount(pData);
        Arc.SerializeArraySize(Count);

        if (Arc.IsReader())
            Resize(pData, Count);

        for (uint32_t ItemIdx = 0; ItemIdx < Count; ItemIdx++)
        {
            if (Arc.ParamBegin("ArrayElement", 0))
            {
                void* pItemData = ItemPointer(pData, ItemIdx);
                mpItemArchetype->SerializeValue(pItemData, Arc);
                Arc.ParamEnd();
            }
        }
    }

    void InitFromArchetype(IProperty* pOther) override
    {
        TTypedProperty::InitFromArchetype(pOther);
        auto* pOtherArray = static_cast<CArrayProperty*>(pOther);
        mpItemArchetype.reset(IProperty::CreateCopy(pOtherArray->mpItemArchetype.get()));
    }

    void PostInitialize() override
    {
        TTypedProperty::PostInitialize();
        mpItemArchetype->Initialize(this, mpScriptTemplate, 0);
    }

    uint32_t ArrayCount(const void* pPropertyData) const
    {
        return ValueRef(pPropertyData);
    }

    void Resize(void* pPropertyData, uint32_t NewCount)
    {
        const uint32_t OldCount = _InternalArrayCount(pPropertyData);
        if (OldCount == NewCount)
            return;

        SScriptArray& rArray = _GetInternalArray(pPropertyData);

        // Handle destruction of old elements
        if (OldCount > NewCount)
        {
            for (uint32_t ItemIdx = NewCount; ItemIdx < OldCount; ItemIdx++)
            {
                void* pItemPtr = ItemPointer(pPropertyData, ItemIdx);
                mpItemArchetype->Destruct(pItemPtr);
            }
        }

        const uint32_t NewSize = NewCount * ItemSize();
        rArray.Array.resize(NewSize);
        rArray.Count = NewCount;

        // Handle construction of new elements
        if (NewCount > OldCount)
        {
            for (uint32_t ItemIdx = OldCount; ItemIdx < NewCount; ItemIdx++)
            {
                void* pItemPtr = ItemPointer(pPropertyData, ItemIdx);
                mpItemArchetype->Construct(pItemPtr);
            }
        }
    }

    void* ItemPointer(void* pPropertyData, uint32_t ItemIndex)
    {
        ASSERT(_InternalArrayCount(pPropertyData) > ItemIndex);
        auto& rArray = _GetInternalArray(pPropertyData).Array;
        const uint32_t MyItemSize = ItemSize();
        ASSERT(rArray.size() >= (MyItemSize * (ItemIndex+1)));
        return rArray.data() + (MyItemSize * ItemIndex);
    }
    const void* ItemPointer(const void* pPropertyData, uint32_t ItemIndex) const
    {
        ASSERT(_InternalArrayCount(pPropertyData) > ItemIndex);
        const auto& rArray = _GetInternalArray(pPropertyData).Array;
        const uint32_t MyItemSize = ItemSize();
        ASSERT(rArray.size() >= (MyItemSize * (ItemIndex + 1)));
        return rArray.data() + (MyItemSize * ItemIndex);
    }

    uint32_t ItemSize() const
    {
        const uint32_t ItemAlign = mpItemArchetype->DataAlignment();
        const uint32_t ItemSize = Math::Align(mpItemArchetype->DataSize(), ItemAlign);
        return ItemSize;
    }

    /** Accessors */
    IProperty* ItemArchetype() const { return mpItemArchetype.get(); }
};

#endif // CARRAYPROPERTY_H
