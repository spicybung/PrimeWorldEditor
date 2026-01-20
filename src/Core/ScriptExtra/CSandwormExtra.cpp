#include "Core/ScriptExtra/CSandwormExtra.h"

#include "Core/Resource/Script/CScriptObject.h"
#include "Core/Scene/CScriptAttachNode.h"
#include "Core/Scene/CScriptNode.h"

CSandwormExtra::CSandwormExtra(CScriptObject* pInstance, CScene* pScene, CScriptNode* pParent)
    : CScriptExtra(pInstance, pScene, pParent)
{
    // The back pincers need to be flipped 180 degrees
    for (auto* attachment : pParent->Attachments())
    {
        if (attachment->LocatorName() == "L_back_claw" || attachment->LocatorName() == "R_back_claw")
            attachment->SetRotation(CVector3f(0, 0, 180));
    }

    // Get pincers scale
    mPincersScale = CFloatRef(pInstance->PropertyData(), pInstance->Template()->Properties()->ChildByID(0x3DB583AE));
    if (mPincersScale.IsValid())
        PropertyModified(mPincersScale.Property());
}

void CSandwormExtra::PropertyModified(IProperty* pProp)
{
    if (pProp == mPincersScale)
    {
        for (auto* attachment : mpScriptNode->Attachments())
            attachment->SetScale(CVector3f(mPincersScale));
    }
}
