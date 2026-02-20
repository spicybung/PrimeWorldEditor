#ifndef IMETATRANSITION_H
#define IMETATRANSITION_H

#include "Core/Resource/Animation/IMetaAnimation.h"
#include <memory>
#include <set>

class CResourceStore;
class IInputStream;
class IMetaAnimation;
class IMetaTransition;
enum class EGame;

enum class EMetaTransType
{
    MetaAnim = 0,
    Trans = 1,
    PhaseTrans = 2, // note: structure shared with eMTT_Trans
    Snap = 3,
    Type4 = 4 // MP3 only
};

// Factory class
class CMetaTransFactory
{
public:
    static std::unique_ptr<IMetaTransition> LoadFromStream(CResourceStore* store, IInputStream& rInput, EGame Game);
};

// Base MetaTransition interface
class IMetaTransition
{
public:
    virtual ~IMetaTransition() = default;
    virtual EMetaTransType Type() const = 0;
    virtual void GetUniquePrimitives(std::set<CAnimPrimitive>& rPrimSet) const = 0;
};

// CMetaTransMetaAnim
class CMetaTransMetaAnim : public IMetaTransition
{
    std::unique_ptr<IMetaAnimation> mpAnim;

public:
    CMetaTransMetaAnim(CResourceStore* store, IInputStream& rInput, EGame Game);
    ~CMetaTransMetaAnim() override;
    EMetaTransType Type() const override;
    void GetUniquePrimitives(std::set<CAnimPrimitive>& rPrimSet) const override;
};

// CMetaTransTrans
class CMetaTransTrans : public IMetaTransition
{
    EMetaTransType mType;
    CCharAnimTime mTransDuration;
    bool mUnknownC = false;
    bool mRunA = false;
    uint32_t mFlags = 0;

public:
    CMetaTransTrans(EMetaTransType Type, IInputStream& rInput, EGame Game);
    EMetaTransType Type() const override;
    void GetUniquePrimitives(std::set<CAnimPrimitive>& rPrimSet) const override;
};

// CMetaTransSnap
class CMetaTransSnap : public IMetaTransition
{
public:
    CMetaTransSnap(IInputStream& rInput, EGame Game);
    EMetaTransType Type() const override;
    void GetUniquePrimitives(std::set<CAnimPrimitive>& rPrimSet) const override;
};

// CMetaTransType4
class CMetaTransType4 : public IMetaTransition
{
public:
    CMetaTransType4(IInputStream& rInput, EGame Game);
    EMetaTransType Type() const override;
    void GetUniquePrimitives(std::set<CAnimPrimitive>& rPrimSet) const override;
};

#endif // IMETATRANSITION_H
