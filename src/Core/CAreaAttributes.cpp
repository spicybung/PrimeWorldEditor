#include "Core/CAreaAttributes.h"

#include "Core/GameProject/CResourceStore.h"
#include "Core/Resource/Script/CGameTemplate.h"
#include "Core/Resource/Script/CScriptLayer.h"
#include "Core/Resource/Script/CScriptObject.h"

CAreaAttributes::CAreaAttributes(CScriptObject *pObj)
{
    SetObject(pObj);
}

CAreaAttributes::~CAreaAttributes() = default;

void CAreaAttributes::SetObject(CScriptObject *pObj)
{
    CScriptTemplate* pTemplate = pObj->Template();
    CStructProperty* pProperties = pTemplate->Properties();

    mpObject = pObj;
    mGame = pTemplate->GameTemplate()->Game();
    mNeedSky = CBoolRef(pObj->PropertyData(), pProperties->ChildByIndex(1));

    if (mGame == EGame::Prime)
        mOverrideSky = CAssetRef(pObj->PropertyData(), pProperties->ChildByIndex(7));
    else if (mGame > EGame::Prime)
        mOverrideSky = CAssetRef(pObj->PropertyData(), pProperties->ChildByID(0xD208C9FA));
}

bool CAreaAttributes::IsLayerEnabled() const
{
    return mpObject->Layer()->IsActive();
}

bool CAreaAttributes::IsSkyEnabled() const
{
    return mNeedSky.IsValid() ? mNeedSky.Get() : false;
}

CModel* CAreaAttributes::SkyModel() const
{
    if (!mOverrideSky.IsValid())
        return nullptr;

    return mpObject->Area()->Entry()->ResourceStore()->LoadResource<CModel>(mOverrideSky.Get());
}
