#ifndef CRESOURCESTORE_H
#define CRESOURCESTORE_H

#include "Core/GameProject/CResourceEntry.h"
#include "Core/GameProject/CVirtualDirectory.h"
#include "Core/Resource/EResType.h"
#include <Common/CAssetID.h>
#include <Common/TString.h>

#include <functional>
#include <map>
#include <memory>
#include <ranges>

class CGameExporter;
class CGameProject;
class CResource;

enum class EDatabaseVersion
{
    Initial,
    // Add new versions before this line

    Max,
    Current = Max - 1
};

class CResourceStore
{
    friend class CResourceIterator;

    CGameProject *mpProj = nullptr;
    EGame mGame{EGame::Prime};
    std::unique_ptr<CVirtualDirectory> mpDatabaseRoot;
    std::map<CAssetID, std::unique_ptr<CResourceEntry>> mResourceEntries;
    std::map<CAssetID, CResourceEntry*> mLoadedResources;
    bool mDatabaseCacheDirty = false;

    // Directory paths
    TString mDatabasePath;
    bool mDatabasePathExists = false;

public:
    explicit CResourceStore(const TString& rkDatabasePath);
    explicit CResourceStore(CGameProject *pProject);
    ~CResourceStore();
    bool SerializeDatabaseCache(IArchive& rArc);
    bool LoadDatabaseCache();
    bool SaveDatabaseCache();
    void ConditionalSaveStore();
    void SetProject(CGameProject *pProj);
    void CloseProject();

    CVirtualDirectory* GetVirtualDirectory(const TString& rkPath, bool AllowCreate);
    void CreateVirtualDirectory(const TString& rkPath);
    void ConditionalDeleteDirectory(CVirtualDirectory *pDir, bool Recurse);
    TString DefaultResourceDirPath() const;
    TString DeletedResourcePath() const;

    bool IsResourceRegistered(const CAssetID& rkID) const;
    CResourceEntry* CreateNewResource(const CAssetID& rkID, EResourceType Type, const TString& rkDir, const TString& rkName, bool ExistingResource = false);
    CResourceEntry* FindEntry(const CAssetID& rkID) const;
    CResourceEntry* FindEntry(const TString& rkPath) const;
    bool AreAllEntriesValid() const;
    void ClearDatabase();
    bool BuildFromDirectory(bool ShouldGenerateCacheFile);
    void RebuildFromDirectory();

    template<typename ResType> ResType* LoadResource(const CAssetID& rkID)  { return static_cast<ResType*>(LoadResource(rkID, ResType::StaticType())); }
    CResource* LoadResource(const CAssetID& rkID);
    CResource* LoadResource(const CAssetID& rkID, EResourceType Type);
    CResource* LoadResource(const TString& rkPath);
    void TrackLoadedResource(CResourceEntry *pEntry);
    void DestroyUnreferencedResources();
    bool DeleteResourceEntry(CResourceEntry *pEntry);

    void ImportNamesFromPakContentsTxt(const TString& rkTxtPath, bool UnnamedOnly);

    static bool IsValidResourcePath(const TString& rkPath, const TString& rkName);
    static TString StaticDefaultResourceDirPath(EGame Game);

    // Accessors
    CGameProject* Project() const            { return mpProj; }
    EGame Game() const                       { return mGame; }
    const TString& DatabaseRootPath() const  { return mDatabasePath; }
    bool DatabasePathExists() const          { return mDatabasePathExists; }
    TString ResourcesDir() const             { return IsEditorStore() ? TString(DatabaseRootPath()) : DatabaseRootPath() + "Resources/"; }
    TString DatabasePath() const             { return DatabaseRootPath() + "ResourceDatabaseCache.bin"; }
    CVirtualDirectory* RootDirectory() const { return mpDatabaseRoot.get(); }
    uint32_t NumTotalResources() const       { return mResourceEntries.size(); }
    uint32_t NumLoadedResources() const      { return mLoadedResources.size(); }
    bool IsCacheDirty() const                { return mDatabaseCacheDirty; }

    // Returns a non-owning filter view into resource entries, which allows for lazily evaluating all entries.
    // We only care about entries that aren't marked for deletion.
    auto MakeResourceView() const
    {
        return mResourceEntries
                | std::views::values
                | std::views::filter(std::not_fn(&CResourceEntry::IsMarkedForDeletion));
    }
    // Creates a non-owning filter view for a particular resource type.
    auto MakeTypedResourceView(EResourceType Type) const {
        return mResourceEntries
                | std::views::values
                | std::views::filter([Type](const auto& entry) { return !entry->IsMarkedForDeletion() && entry->ResourceType() == Type; });
    }

    void SetCacheDirty()       { mDatabaseCacheDirty = true; }
    bool IsEditorStore() const { return mpProj == nullptr; }
};

extern TString gDataDir;
extern bool gResourcesWritable;
extern bool gTemplatesWritable;
extern CResourceStore *gpResourceStore;
extern CResourceStore *gpEditorStore;

inline auto MakeResourceView(CResourceStore* store = gpResourceStore)
{
    return store->MakeResourceView();
}
inline auto MakeTypedResourceView(EResourceType Type, CResourceStore* store = gpResourceStore)
{
    return store->MakeTypedResourceView(Type);
}

#endif // CRESOURCESTORE_H
