#include "Core/Resource/Animation/IMetaTransition.h"

#include "Core/Resource/Animation/IMetaAnimation.h"
#include <Common/Log.h>

// ************ CMetaTransFactory ************

std::unique_ptr<IMetaTransition> CMetaTransFactory::LoadFromStream(CResourceStore* store, IInputStream& rInput, EGame Game)
{
    const auto Type = static_cast<EMetaTransType>(rInput.ReadU32());

    switch (Type)
    {
    case EMetaTransType::MetaAnim:
        return std::make_unique<CMetaTransMetaAnim>(store, rInput, Game);

    case EMetaTransType::Trans:
    case EMetaTransType::PhaseTrans:
        return std::make_unique<CMetaTransTrans>(Type, rInput, Game);

    case EMetaTransType::Snap:
        return std::make_unique<CMetaTransSnap>(rInput, Game);

    case EMetaTransType::Type4:
        return std::make_unique<CMetaTransType4>(rInput, Game);

    default:
        NLog::Error("Unrecognized meta-transition type: {}", static_cast<int>(Type));
        return nullptr;
    }
}

// ************ CMetaTransMetaAnim ************
CMetaTransMetaAnim::CMetaTransMetaAnim(CResourceStore* store, IInputStream& rInput, EGame Game)
    : mpAnim{CMetaAnimFactory::LoadFromStream(store, rInput, Game)}
{
}

CMetaTransMetaAnim::~CMetaTransMetaAnim() = default;

EMetaTransType CMetaTransMetaAnim::Type() const
{
    return EMetaTransType::MetaAnim;
}

void CMetaTransMetaAnim::GetUniquePrimitives(std::set<CAnimPrimitive>& rPrimSet) const
{
    mpAnim->GetUniquePrimitives(rPrimSet);
}

// ************ CMetaTransTrans ************
CMetaTransTrans::CMetaTransTrans(EMetaTransType Type, IInputStream& rInput, EGame Game)
{
    ASSERT(Type == EMetaTransType::Trans || Type == EMetaTransType::PhaseTrans);
    mType = Type;

    if (Game <= EGame::Echoes)
    {
        mTransDuration.SetTime(rInput.ReadF32());
        mTransDuration.SetType(static_cast<CCharAnimTime::EType>(rInput.ReadU32()));
        mUnknownC = rInput.ReadBool();
        mRunA = rInput.ReadBool();
        mFlags = rInput.ReadU32();
    }
    else
    {
        rInput.Skip(0x13);
    }
}

EMetaTransType CMetaTransTrans::Type() const
{
    return mType;
}

void CMetaTransTrans::GetUniquePrimitives(std::set<CAnimPrimitive>&) const
{
}

// ************ CMetaTransSnap ************
CMetaTransSnap::CMetaTransSnap(IInputStream&, EGame)
{
}

EMetaTransType CMetaTransSnap::Type() const
{
    return EMetaTransType::Snap;
}

void CMetaTransSnap::GetUniquePrimitives(std::set<CAnimPrimitive>&) const
{
}

// ************ CMetaTransType4 ************
CMetaTransType4::CMetaTransType4(IInputStream& rInput, EGame)
{
    rInput.Skip(0x14);
}

EMetaTransType CMetaTransType4::Type() const
{
    return EMetaTransType::Type4;
}

void CMetaTransType4::GetUniquePrimitives(std::set<CAnimPrimitive>&) const
{
}
