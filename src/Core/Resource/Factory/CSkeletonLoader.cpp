#include "Core/Resource/Factory/CSkeletonLoader.h"

#include <Common/Macros.h>
#include <Common/Log.h>
#include <Common/Math/CQuaternion.h>
#include "Core/Resource/Animation/CSkeleton.h"

#include <vector>

void CSkeletonLoader::SetLocalBoneCoords(CBone *pBone)
{
    for (size_t iChild = 0; iChild < pBone->NumChildren(); iChild++)
        SetLocalBoneCoords(pBone->ChildByIndex(iChild));

    if (pBone->mpParent != nullptr)
        pBone->mLocalPosition = pBone->mPosition - pBone->mpParent->mPosition;
    else
        pBone->mLocalPosition = pBone->mPosition;
}

void CSkeletonLoader::CalculateBoneInverseBindMatrices()
{
    for (auto& bone : mpSkeleton->mBones)
    {
        bone->mInvBind = CTransform4f::TranslationMatrix(-bone->Position());
    }
}

// ************ STATIC ************
std::unique_ptr<CSkeleton> CSkeletonLoader::LoadCINF(IInputStream& rCINF, CResourceEntry *pEntry)
{
    auto ptr = std::make_unique<CSkeleton>(pEntry);

    CSkeletonLoader Loader;
    Loader.mpSkeleton = ptr.get();
    EGame Game = pEntry->Game();

    // We don't support DKCR CINF right now
    if (rCINF.PeekU32() == 0x9E220006)
        return ptr;

    const auto NumBones = rCINF.ReadU32();
    ptr->mBones.reserve(NumBones);

    // Read bones
    struct SBoneInfo
    {
        uint32_t ParentID{};
        std::vector<uint32_t> ChildIDs;
    };
    std::vector<SBoneInfo> BoneInfo(NumBones);

    for (uint32_t iBone = 0; iBone < NumBones; iBone++)
    {
        auto& pBone = ptr->mBones.emplace_back(std::make_unique<CBone>(ptr.get()));

        pBone->mID = rCINF.ReadU32();
        BoneInfo[iBone].ParentID = rCINF.ReadU32();
        pBone->mPosition = CVector3f(rCINF);

        // Version test. No version number. The next value is the linked bone count in MP1 and the
        // rotation value in MP2. The max bone count is 100 so the linked bone count will not be higher
        // than that. Additionally, every bone links to its parent at least and every skeleton (as far as I
        // know) has at least two bones so the linked bone count will never be 0.
        if (Game == EGame::Invalid)
        {
            const auto Check = rCINF.PeekU32();
            Game = ((Check > 100 || Check == 0) ? EGame::Echoes : EGame::Prime);
        }
        if (Game >= EGame::Echoes)
        {
            pBone->mRotation = CQuaternion(rCINF);
            pBone->mLocalRotation = CQuaternion(rCINF);
        }

        const auto NumLinkedBones = rCINF.ReadU32();
        ASSERT(NumLinkedBones != 0);

        for (uint32_t iLink = 0; iLink < NumLinkedBones; iLink++)
        {
            const auto LinkedID = rCINF.ReadU32();

            if (LinkedID != BoneInfo[iBone].ParentID)
                BoneInfo[iBone].ChildIDs.push_back(LinkedID);
        }
    }

    // Fill in bone info
    for (uint32_t iBone = 0; iBone < NumBones; iBone++)
    {
        CBone *pBone = ptr->mBones[iBone].get();
        const SBoneInfo& rInfo = BoneInfo[iBone];

        pBone->mpParent = ptr->BoneByID(rInfo.ParentID);

        for (const auto childID : rInfo.ChildIDs)
        {
            if (CBone* pChild = ptr->BoneByID(childID))
                pBone->mChildren.push_back(pChild);
            else
                NLog::Error("{}: Bone {} has invalid child ID: {}", rCINF.GetSourceString(), pBone->mID, childID);
        }

        if (pBone->mpParent == nullptr)
        {
            if (ptr->mpRootBone == nullptr)
                ptr->mpRootBone = pBone;
            else
                NLog::Error("{}: Multiple root bones?", rCINF.GetSourceString());
        }
    }

    Loader.SetLocalBoneCoords(ptr->mpRootBone);
    Loader.CalculateBoneInverseBindMatrices();

    // Skip bone ID array
    const auto NumBoneIDs = rCINF.ReadU32();
    rCINF.Seek(NumBoneIDs * 4, SEEK_CUR);

    // Read bone names
    const auto NumBoneNames = rCINF.ReadU32();

    for (uint32_t iName = 0; iName < NumBoneNames; iName++)
    {
        TString Name = rCINF.ReadString();
        const auto BoneID = rCINF.ReadU32();

        ptr->BoneByID(BoneID)->mName = std::move(Name);
    }

    return ptr;
}
