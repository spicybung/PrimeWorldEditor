#ifndef CVECTORPROPERTY_H
#define CVECTORPROPERTY_H

#include <Common/Math/CVector3f.h>
#include "Core/Resource/Script/Property/IProperty.h"

class CVectorProperty : public TSerializeableTypedProperty<CVector3f, EPropertyType::Vector>
{
    friend class IProperty;

protected:
    explicit CVectorProperty(EGame Game)
        : TSerializeableTypedProperty(Game)
    {}

public:
    void PostInitialize() override
    {
        CreateIntrinsic(EPropertyType::Float, this, mOffset + 0, "X");
        CreateIntrinsic(EPropertyType::Float, this, mOffset + 4, "Y");
        CreateIntrinsic(EPropertyType::Float, this, mOffset + 8, "Z");
    }

    void SerializeValue(void* pData, IArchive& Arc) override
    {
        ValueRef(pData).Serialize(Arc);
    }

    TString ValueAsString(const void* pData) const override
    {
        return ValueRef(pData).ToString();
    }
};

#endif // CVECTORPROPERTY_H
