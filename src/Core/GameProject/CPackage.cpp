#include "Core/GameProject/CPackage.h"

#include "Core/CompressionUtil.h"
#include "Core/IProgressNotifier.h"
#include "Core/GameProject/DependencyListBuilders.h"
#include "Core/GameProject/CGameProject.h"
#include "Core/Resource/Cooker/CWorldCooker.h"
#include <Common/CScopedTimer.h>
#include <Common/Macros.h>
#include <Common/FileUtil.h>
#include <Common/FileIO/CFileInStream.h>
#include <Common/FileIO/CFileOutStream.h>
#include <Common/Serialization/CXMLReader.h>
#include <Common/Serialization/CXMLWriter.h>

using namespace tinyxml2;

CPackage::CPackage() = default;

CPackage::CPackage(CGameProject* pProj, TString rkName, TString rkPath)
    : mpProject(pProj)
    , mPakName(std::move(rkName))
    , mPakPath(std::move(rkPath))
    , mCacheDirty(true)
{
}

CPackage::~CPackage() = default;

bool CPackage::Load()
{
    const TString DefPath = DefinitionPath(false);
    CXMLReader Reader(DefPath);

    if (Reader.IsValid())
    {
        Serialize(Reader);
        mCacheDirty = true;
        return true;
    }

    return false;
}

bool CPackage::Save()
{
    const TString DefPath = DefinitionPath(false);
    FileUtil::MakeDirectory(DefPath.GetFileDirectory());

    CXMLWriter Writer(DefPath, "PackageDefinition", 0, mpProject ? mpProject->Game() : EGame::Invalid);
    Serialize(Writer);
    return Writer.Save();
}

void CPackage::Serialize(IArchive& rArc)
{
    rArc << SerialParameter("NeedsRecook", mNeedsRecook)
         << SerialParameter("NamedResources", mResources);
}

void CPackage::AddResource(TString rkName, const CAssetID& rkID, const CFourCC& rkType)
{
    mResources.emplace_back(std::move(rkName), rkID, rkType);
    mCacheDirty = true;
}

void CPackage::UpdateDependencyCache() const
{
    CPackageDependencyListBuilder Builder(this);
    std::list<CAssetID> AssetList;
    Builder.BuildDependencyList(false, AssetList);

    mCachedDependencies.clear();
    for (const auto& asset : AssetList)
        mCachedDependencies.insert(asset);

    mCacheDirty = false;
}

void CPackage::MarkDirty()
{
    if (!mNeedsRecook)
    {
        mNeedsRecook = true;
        Save();
        UpdateDependencyCache();
    }
}

void CPackage::Cook(IProgressNotifier *pProgress)
{
    SCOPED_TIMER(CookPackage);

    // Build asset list
    pProgress->Report(-1, -1, "Building dependency list");

    CPackageDependencyListBuilder Builder(this);
    std::list<CAssetID> AssetList;
    Builder.BuildDependencyList(true, AssetList);
    NLog::Debug("{} assets in {}.pak", AssetList.size(), Name());

    // Write new pak
    const TString PakPath = CookedPackagePath(false);
    CFileOutStream Pak(PakPath, std::endian::big);

    if (!Pak.IsValid())
    {
        NLog::Error("Couldn't cook package {}; unable to open package for writing", CookedPackagePath(true));
        return;
    }

    const EGame Game = mpProject->Game();
    const uint32_t Alignment = (Game <= EGame::CorruptionProto ? 0x20 : 0x40);
    const uint32_t AlignmentMinusOne = Alignment - 1;

    uint32_t TocOffset = 0;
    uint32_t NamesSize = 0;
    uint32_t ResTableOffset = 0;
    uint32_t ResTableSize = 0;
    uint32_t ResDataSize = 0;

    // Write MP1 pak header
    if (Game <= EGame::CorruptionProto)
    {
        Pak.WriteU32(0x00030005); // Major/Minor Version
        Pak.WriteU32(0); // Unknown

        // Named Resources
        Pak.WriteU32(mResources.size());

        for (const auto& res : mResources)
        {
            res.Type.Write(Pak);
            res.ID.Write(Pak);
            Pak.WriteSizedString(res.Name);
        }
    }
    else // Write MP3 pak header
    {
        // Header
        Pak.WriteU32(2); // Version
        Pak.WriteU32(0x40); // Header size
        Pak.WriteToBoundary(0x40, 0); // We don't care about the MD5 hash; the game doesn't use it

        // PAK table of contents; write later
        TocOffset = Pak.Tell();
        Pak.WriteU32(0);
        Pak.WriteToBoundary(0x40, 0);

        // Named Resources
        const auto NamesStart = Pak.Tell();
        Pak.WriteU32(static_cast<uint32_t>(mResources.size()));

        for (const auto& res : mResources)
        {
            Pak.WriteString(res.Name);
            res.Type.Write(Pak);
            res.ID.Write(Pak);
        }

        Pak.WriteToBoundary(0x40, 0);
        NamesSize = Pak.Tell() - NamesStart;
    }

    // Fill in resource table with junk, write later
    ResTableOffset = Pak.Tell();
    Pak.WriteU32(AssetList.size());
    const CAssetID Dummy = CAssetID::InvalidID(Game);

    for (size_t iRes = 0; iRes < AssetList.size(); iRes++)
    {
        Pak.WriteU64(0);
        Dummy.Write(Pak);
        Pak.WriteU64(0);
    }

    Pak.WriteToBoundary(Alignment, 0);
    ResTableSize = Pak.Tell() - ResTableOffset;

    // Start writing resources
    struct SResourceTableInfo
    {
        CResourceEntry* pEntry{};
        uint32_t Offset{};
        uint32_t Size{};
        bool Compressed{};
    };
    std::vector<SResourceTableInfo> ResourceTableData(AssetList.size());
    uint32_t ResIdx = 0;
    const uint32_t ResDataOffset = Pak.Tell();

    for (auto Iter = AssetList.begin(); Iter != AssetList.end() && !pProgress->ShouldCancel(); Iter++, ResIdx++)
    {
        // Initialize entry, recook assets if needed
        const uint32_t AssetOffset = Pak.Tell();
        const CAssetID ID = *Iter;
        CResourceEntry* pEntry = mpProject->ResourceStore()->FindEntry(ID);
        ASSERT(pEntry != nullptr);

        if (pEntry->NeedsRecook())
        {
            pProgress->Report(ResIdx, AssetList.size(), fmt::format("Cooking asset: {}.{}", pEntry->Name(), pEntry->CookedExtension().ToString()));
            pEntry->Cook();
        }

        // Update progress bar
        if ((ResIdx & 1) != 0 || ResIdx == AssetList.size() - 1)
        {
            pProgress->Report(ResIdx, AssetList.size(), fmt::format("Writing asset {}/{}: {}", ResIdx+1, AssetList.size(), (pEntry->Name() + "." + pEntry->CookedExtension())));
        }

        // Update table info
        SResourceTableInfo& rTableInfo = ResourceTableData[ResIdx];
        rTableInfo.pEntry = pEntry;
        rTableInfo.Offset = (Game <= EGame::Echoes ? AssetOffset : AssetOffset - ResDataOffset);

        // Load resource data
        CFileInStream CookedAsset(pEntry->CookedAssetPath(), std::endian::big);
        ASSERT(CookedAsset.IsValid());
        const uint32_t ResourceSize = CookedAsset.Size();

        std::vector<uint8_t> ResourceData(ResourceSize);
        CookedAsset.ReadBytes(ResourceData.data(), ResourceData.size());

        // Check if this asset should be compressed; there are a few resource types that are
        // always compressed, and some types that are compressed if they're over a certain size
        const EResourceType Type = pEntry->ResourceType();
        const uint32_t CompressThreshold = (Game <= EGame::CorruptionProto ? 0x400 : 0x80);

        bool ShouldAlwaysCompress = (Type == EResourceType::Texture || Type == EResourceType::Model ||
                                     Type == EResourceType::Skin || Type == EResourceType::AnimSet ||
                                     Type == EResourceType::Animation || Type == EResourceType::Font);

        if (Game >= EGame::Corruption)
        {
            ShouldAlwaysCompress = ShouldAlwaysCompress ||
                                   (Type == EResourceType::Character || Type == EResourceType::SourceAnimData ||
                                    Type == EResourceType::Scan || Type == EResourceType::AudioSample ||
                                    Type == EResourceType::StringTable || Type == EResourceType::AudioAmplitudeData ||
                                    Type == EResourceType::DynamicCollision);
        }

        const bool ShouldCompressConditional = !ShouldAlwaysCompress &&
                                               (Type == EResourceType::Particle || Type == EResourceType::ParticleElectric ||
                                                Type == EResourceType::ParticleSwoosh || Type == EResourceType::ParticleWeapon ||
                                                Type == EResourceType::ParticleDecal || Type == EResourceType::ParticleCollisionResponse ||
                                                Type == EResourceType::ParticleSpawn || Type == EResourceType::ParticleSorted ||
                                                Type == EResourceType::BurstFireData);

        const bool ShouldCompress = ShouldAlwaysCompress || (ShouldCompressConditional && ResourceSize >= CompressThreshold);

        // Write resource data to pak
        if (!ShouldCompress)
        {
            Pak.WriteBytes(ResourceData.data(), ResourceSize);
            rTableInfo.Compressed = false;
        }
        else
        {
            uint32_t CompressedSize;
            std::vector<uint8_t> CompressedData(ResourceData.size() * 2);
            bool Success = false;

            if (Game <= EGame::EchoesDemo || Game == EGame::DKCReturns)
                Success = CompressionUtil::CompressZlib(ResourceData.data(), ResourceData.size(), CompressedData.data(), CompressedData.size(), CompressedSize);
            else
                Success = CompressionUtil::CompressLZOSegmented(ResourceData.data(), ResourceData.size(), CompressedData.data(), CompressedSize, false);

            // Make sure that the compressed data is actually smaller, accounting for padding + uncompressed size value
            if (Success)
            {
                const uint32_t CompressionHeaderSize = (Game <= EGame::CorruptionProto ? 4 : 0x10);
                const uint32_t PaddedUncompressedSize = (ResourceSize + AlignmentMinusOne) & ~AlignmentMinusOne;
                const uint32_t PaddedCompressedSize = (CompressedSize + CompressionHeaderSize + AlignmentMinusOne) & ~AlignmentMinusOne;
                Success = (PaddedCompressedSize < PaddedUncompressedSize);
            }

            // Write file to pak
            if (Success)
            {
                // Write MP1/2 compressed asset
                if (Game <= EGame::CorruptionProto)
                {
                    Pak.WriteU32(ResourceSize);
                }
                // Write MP3/DKCR compressed asset
                else
                {
                    // Note: Compressed asset data can be stored in multiple blocks. Normally, the only assets that make use of this are textures,
                    // which can store each separate component of the file (header, palette, image data) in separate blocks. However, some textures
                    // are stored in one block, and I've had no luck figuring out why. The game doesn't generally seem to care whether textures use
                    // multiple blocks or not, so for the sake of simplicity we compress everything to one block.
                    Pak.WriteFourCC(CFourCC("CMPD"));
                    Pak.WriteU32(1);
                    Pak.WriteU32(0xA0000000 | CompressedSize);
                    Pak.WriteU32(ResourceSize);
                }
                Pak.WriteBytes(CompressedData.data(), CompressedSize);
            }
            else
            {
                Pak.WriteBytes(ResourceData.data(), ResourceSize);
            }

            rTableInfo.Compressed = Success;
        }

        Pak.WriteToBoundary(Alignment, 0xFF);
        rTableInfo.Size = Pak.Tell() - AssetOffset;
    }
    ResDataSize = Pak.Tell() - ResDataOffset;

    // If we cancelled, don't finish writing the pak; delete the file instead and make sure the package is flagged for recook
    if (pProgress->ShouldCancel())
    {
        Pak.Close();
        FileUtil::DeleteFile(PakPath);
        mNeedsRecook = true;
    }
    else
    {
        // Write table of contents for real
        if (Game >= EGame::Corruption)
        {
            Pak.Seek(TocOffset, SEEK_SET);
            Pak.WriteU32(3); // Always 3 pak sections
            Pak.WriteFourCC(CFourCC("STRG"));
            Pak.WriteU32(NamesSize);
            Pak.WriteFourCC(CFourCC("RSHD"));
            Pak.WriteU32(ResTableSize);
            Pak.WriteFourCC(CFourCC("DATA"));
            Pak.WriteU32(ResDataSize);
        }

        // Write resource table for real
        Pak.Seek(ResTableOffset+4, SEEK_SET);

        for (size_t iRes = 0; iRes < AssetList.size(); iRes++)
        {
            const SResourceTableInfo& rkInfo = ResourceTableData[iRes];
            CResourceEntry *pEntry = rkInfo.pEntry;

            Pak.WriteU32(rkInfo.Compressed ? 1 : 0);
            pEntry->CookedExtension().Write(Pak);
            pEntry->ID().Write(Pak);
            Pak.WriteU32(rkInfo.Size);
            Pak.WriteU32(rkInfo.Offset);
        }

        // Clear recook flag
        mNeedsRecook = false;
        NLog::Debug("Finished writing {}", PakPath);
    }

    Save();

    // Update resource store in case we recooked any assets
    mpProject->ResourceStore()->ConditionalSaveStore();
}

void CPackage::CompareOriginalAssetList(const std::list<CAssetID>& rkNewList)
{
    // Debug - take the newly generated rkNewList and compare it with the asset list
    // from the original pak, and print info about any extra or missing resources
    // Build a set out of the generated list
    const std::set<CAssetID> NewListSet(rkNewList.begin(), rkNewList.end());

    // Read the original pak
    const TString CookedPath = CookedPackagePath(false);
    CFileInStream Pak(CookedPath, std::endian::big);

    if (!Pak.IsValid() || Pak.Size() == 0)
    {
        NLog::Error("Failed to compare to original asset list; couldn't open the original pak");
        return;
    }

    // Determine pak version
    const auto PakVersion = Pak.ReadU32();
    std::set<CAssetID> OldListSet;

    // Read MP1/2 pak
    if (PakVersion == 0x00030005)
    {
        Pak.Seek(0x4, SEEK_CUR);
        const auto NumNamedResources = Pak.ReadU32();

        for (uint32_t iName = 0; iName < NumNamedResources; iName++)
        {
            Pak.Seek(0x8, SEEK_CUR);
            const auto NameLen = Pak.ReadU32();
            Pak.Seek(NameLen, SEEK_CUR);
        }

        // Build a set out of the original pak resource list
        const auto NumResources = Pak.ReadU32();

        for (uint32_t iRes = 0; iRes < NumResources; iRes++)
        {
            Pak.Seek(0x8, SEEK_CUR);
            OldListSet.insert(CAssetID(Pak, EIDLength::k32Bit));
            Pak.Seek(0x8, SEEK_CUR);
        }
    }
    else // Read MP3/DKCR pak
    {
        ASSERT(PakVersion == 0x2);

        // Skip named resources
        Pak.Seek(0x44, SEEK_SET);
        [[maybe_unused]] const auto StringSecType = CFourCC(Pak.ReadU32());
        const auto StringSecSize = Pak.ReadU32();
        ASSERT(StringSecType == CFourCC("STRG"));

        Pak.Seek(0x80 + StringSecSize, SEEK_SET);

        // Read resource table
        const auto NumResources = Pak.ReadU32();

        for (uint32_t iRes = 0; iRes < NumResources; iRes++)
        {
            Pak.Seek(0x8, SEEK_CUR);
            OldListSet.insert(CAssetID(Pak, EIDLength::k64Bit));
            Pak.Seek(0x8, SEEK_CUR);
        }
    }

    // Check for missing resources in the new list
    for (const auto& ID : OldListSet)
    {
        if (!NewListSet.contains(ID))
        {
            const CResourceEntry* pEntry = mpProject->ResourceStore()->FindEntry(ID);
            const TString Extension = (pEntry != nullptr ? "." + pEntry->CookedExtension() : "");
            NLog::Warn("Missing resource: {}{}", ID.ToString(), Extension);
        }
    }

    // Check for extra resources in the new list
    for (const auto& ID : NewListSet)
    {
        if (!OldListSet.contains(ID))
        {
            const CResourceEntry *pEntry = mpProject->ResourceStore()->FindEntry(ID);
            const TString Extension = (pEntry != nullptr ? "." + pEntry->CookedExtension() : "");
            NLog::Warn("Extra resource: {}{}", ID.ToString(), Extension);
        }
    }
}

bool CPackage::ContainsAsset(const CAssetID& rkID) const
{
    if (mCacheDirty)
        UpdateDependencyCache();

    return mCachedDependencies.contains(rkID);
}

TString CPackage::DefinitionPath(bool Relative) const
{
    TString RelPath = mPakPath + mPakName + ".pkd";
    return Relative ? RelPath : mpProject->PackagesDir(false) + RelPath;
}

TString CPackage::CookedPackagePath(bool Relative) const
{
    TString RelPath = mPakPath + mPakName + ".pak";
    return Relative ? RelPath : mpProject->DiscFilesystemRoot(false) + RelPath;
}
