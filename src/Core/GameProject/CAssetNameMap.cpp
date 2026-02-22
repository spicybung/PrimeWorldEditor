#include "Core/GameProject/CAssetNameMap.h"

#include "Core/GameProject/CResourceEntry.h"
#include "Core/GameProject/CResourceStore.h"

#include <Common/CFourCC.h>
#include <Common/Log.h>
#include <Common/Macros.h>
#include <Common/Serialization/CXMLReader.h>
#include <Common/Serialization/CXMLWriter.h>

constexpr char gkAssetMapPath[] = "resources/gameinfo/AssetNameMap";
constexpr char gkAssetMapExt[] = "xml";

struct CAssetNameMap::SAssetNameInfo
{
    TString Name;
    TString Directory;
    CFourCC Type; // This is mostly just needed to verify no name conflicts
    bool AutoGenName{};
    bool AutoGenDir{};

    TString FullPath() const
    {
        return Directory + Name + '.' + Type.ToString();
    }

    void Serialize(IArchive& rArc)
    {
        rArc << SerialParameter("Name", Name)
             << SerialParameter("Directory", Directory)
             << SerialParameter("Type", Type)
             << SerialParameter("AutoGenName", AutoGenName)
             << SerialParameter("AutoGenDir", AutoGenDir);
    }

    bool operator<(const SAssetNameInfo& rkOther) const
    {
        return FullPath().ToUpper() < rkOther.FullPath().ToUpper();
    }

    bool operator==(const SAssetNameInfo& rkOther) const
    {
        return FullPath().CaseInsensitiveCompare(rkOther.FullPath());
    }
};

CAssetNameMap::CAssetNameMap(EIDLength IDLength)
    : mIDLength(IDLength)
{
    ASSERT(mIDLength != kInvalidIDLength);
}

CAssetNameMap::CAssetNameMap(EGame Game) 
    : mIDLength(CAssetID::GameIDLength(Game))
{
    ASSERT(mIDLength != kInvalidIDLength);
}

CAssetNameMap::~CAssetNameMap() = default;

bool CAssetNameMap::LoadAssetNames(TString Path)
{
    if (Path.IsEmpty())
        Path = DefaultNameMapPath(mIDLength);

    CXMLReader Reader(Path);

    if (Reader.IsValid())
    {
        const auto FileIDLength = CAssetID::GameIDLength(Reader.Game());

        if (FileIDLength == mIDLength)
        {
            Serialize(Reader);
            return true;
        }
        else
        {
            NLog::Debug("Failed to load asset names; expected {} IDs, got {}",
                        mIDLength    == EIDLength::k32Bit ? "32-bit" : "64-bit",
                        FileIDLength == EIDLength::k32Bit ? "32-bit" : "64-bit"  );
            return false;
        }
    }

    NLog::Error("Failed to load asset names; couldn't open XML.");
    return false;
}

bool CAssetNameMap::SaveAssetNames(TString Path)
{
    if (Path.IsEmpty())
        Path = DefaultNameMapPath(mIDLength);

    EGame Game = (mIDLength == EIDLength::k32Bit ? EGame::Prime : EGame::Corruption);
    CXMLWriter Writer(Path, "AssetNameMap", 0, Game);
    Serialize(Writer);
    return Writer.Save();
}

bool CAssetNameMap::GetNameInfo(const CAssetID& ID, TString& rOutDirectory, TString& rOutName, bool& rOutAutoGenDir, bool& rOutAutoGenName)
{
    const auto It = mMap.find(ID);

    if (It != mMap.end())
    {
        const SAssetNameInfo& rInfo = It->second;
        rOutName = rInfo.Name;
        rOutDirectory = rInfo.Directory;
        rOutAutoGenDir = rInfo.AutoGenDir;
        rOutAutoGenName = rInfo.AutoGenName;
        return true;
    }
    else
    {
        const EGame Game = (ID.Length() == EIDLength::k32Bit ? EGame::Prime : EGame::Corruption);
        rOutDirectory = CResourceStore::StaticDefaultResourceDirPath(Game);
        rOutName = ID.ToString();
        rOutAutoGenDir = true;
        rOutAutoGenName = true;
        return false;
    }
}

void CAssetNameMap::CopyFromStore(CResourceStore *pStore)
{
    // Do a first pass to remove old paths from used set to prevent false positives from eg. if two resources switch places
    ASSERT(CAssetID::GameIDLength(pStore->Game()) == mIDLength);

    for (const auto& It : pStore->MakeResourceView())
    {
        if (It->IsCategorized() || It->IsNamed())
        {
            const auto Find = mMap.find(It->ID());

            if (Find != mMap.end())
            {
                const SAssetNameInfo& rInfo = Find->second;
                const auto UsedFind = mUsedSet.find(rInfo);
                ASSERT(UsedFind != mUsedSet.end());
                mUsedSet.erase(UsedFind);
            }
        }
    }

    // Do a second pass to add the new paths to the map
    for (const auto& It : pStore->MakeResourceView())
    {
        if (It->IsCategorized() || It->IsNamed())
        {
            const CAssetID& ID = It->ID();
            ASSERT(ID.Length() == mIDLength);

            SAssetNameInfo NameInfo{
                .Name = It->Name(),
                .Directory = It->Directory()->FullPath(),
                .Type = It->CookedExtension(),
                .AutoGenName = It->HasFlag(EResEntryFlag::AutoResName),
                .AutoGenDir = It->HasFlag(EResEntryFlag::AutoResDir),
            };

            // Check for conflicts with new name
            if (mUsedSet.contains(NameInfo))
            {
                SAssetNameInfo NewNameInfo = NameInfo;
                int NumConflicted = 0;

                while (mUsedSet.contains(NewNameInfo))
                {
                    NewNameInfo.Name = fmt::format("{}_{}", NameInfo.Name, NumConflicted);
                    NumConflicted++;
                }

                NLog::Warn("Detected name conflict when copying asset name from the resource store; renaming.");
                NLog::Warn("\tOld Path: {}", NameInfo.FullPath());
                NLog::Warn("\tNew Path: {}", NewNameInfo.FullPath());
                NameInfo.Name = NewNameInfo.Name;
            }

            // Assign to map
            mMap.insert_or_assign(ID, NameInfo);
            mUsedSet.insert(std::move(NameInfo));
        }
    }
}

// ************ PRIVATE ************
void CAssetNameMap::Serialize(IArchive& rArc)
{
    rArc << SerialParameter("AssetNameMap", mMap);

    if (rArc.IsReader())
        PostLoadValidate();
}

void CAssetNameMap::PostLoadValidate()
{
    // Make sure the newly loaded map doesn't contain any errors or name conflicts.
    bool FoundErrors = false;
    mIsValid = false;
    std::set<SAssetNameInfo> Dupes;

    for (auto Iter = mMap.begin(); Iter != mMap.end(); ++Iter)
    {
        const SAssetNameInfo& rkInfo = Iter->second;

        if (mUsedSet.contains(rkInfo))
        {
            Dupes.insert(rkInfo);
        }
        else
        {
            mUsedSet.insert(rkInfo);

            // Verify the name/path is valid
            if (!CResourceStore::IsValidResourcePath(rkInfo.Directory, rkInfo.Name))
            {
                NLog::Error("Invalid resource path in asset name map: {}{}.{}", rkInfo.Directory, rkInfo.Name, rkInfo.Type.ToString());
                Iter = mMap.erase(Iter);
                FoundErrors = true;
            }

            // Verify correct ID length
            if (Iter->first.Length() != mIDLength)
            {
                NLog::Error("Incorrect asset ID length in asset name map: {}", Iter->first.ToString());
                Iter = mMap.erase(Iter);
                FoundErrors = true;
            }
        }
    }

    // If we detected any dupes, then this map can't be used
    if (!Dupes.empty())
    {
        NLog::Error("Asset name map is invalid and cannot be used! Duplicate asset entries detected:");

        for (const auto& dupe : Dupes)
        {
            NLog::Warn("\t{}", dupe.FullPath());
        }

        mMap.clear();
    }
    else
    {
        mIsValid = !FoundErrors;
    }
}

TString CAssetNameMap::DefaultNameMapPath(EIDLength IDLength)
{
    ASSERT(IDLength != kInvalidIDLength);
    const char* const Suffix = (IDLength == EIDLength::k32Bit ? "32" : "64");
    return gDataDir + gkAssetMapPath + Suffix + '.' + gkAssetMapExt;
}

TString CAssetNameMap::DefaultNameMapPath(EGame Game)
{
    return DefaultNameMapPath( CAssetID::GameIDLength(Game) );
}

TString CAssetNameMap::GetExtension()
{
    return gkAssetMapExt;
}
