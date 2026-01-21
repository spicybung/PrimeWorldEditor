#include "Core/GameProject/COpeningBanner.h"

#include <Common/FileIO/CFileInStream.h>
#include <Common/FileIO/CFileOutStream.h>
#include <Common/FileIO/CMemoryInStream.h>
#include <Common/FileIO/CMemoryOutStream.h>
#include "Core/GameProject/CGameProject.h"

COpeningBanner::COpeningBanner(CGameProject *pProj)
    : mpProj(pProj)
{
    mWii = mpProj->IsWiiBuild();

    TString BannerPath = mpProj->DiscFilesystemRoot(false) + "opening.bnr";
    CFileInStream Banner(BannerPath, std::endian::big);

    if (Banner.IsValid())
    {
        mBannerData.resize(Banner.Size());
        Banner.ReadBytes(mBannerData.data(), mBannerData.size());
    }
}

TString COpeningBanner::EnglishGameName() const
{
    // opening.bnr stores the game name in a fixed-length buffer. Need to account for
    // this and prevent the string-reading function from overrunning the buffer
    CMemoryInStream Banner(mBannerData.data(), mBannerData.size(), std::endian::big);

    uint32_t CharSize = mWii ? 2 : 1;
    uint32_t MaxLen = MaxGameNameLength();

    std::vector<uint8_t> NameBuffer((MaxLen + 1) * CharSize, 0);
    Banner.GoTo( mWii ? 0xB0 : 0x1860 );
    Banner.ReadBytes(NameBuffer.data(), MaxLen * CharSize);

    Banner.SetData(NameBuffer.data(), NameBuffer.size(), std::endian::big);
    return mWii ? Banner.Read16String().ToUTF8() : Banner.ReadString();
}

void COpeningBanner::SetEnglishGameName(const TString& rkName)
{
    CMemoryOutStream Banner(mBannerData.data(), mBannerData.size(), std::endian::big);
    uint32_t PadCount = 0;

    uint32_t MaxLen = MaxGameNameLength();
    ASSERT(rkName.Size() <= MaxLen);

    if (mWii)
    {
        Banner.GoTo(0xB0);
        Banner.Write16String(rkName.ToUTF16(), -1, false);
        PadCount = (MaxLen - rkName.Size()) * 2;
    }
    else
    {
        Banner.GoTo(0x1860);
        Banner.WriteString(rkName, -1, false);
        PadCount = MaxLen - rkName.Size();
    }

    for (uint32_t Pad = 0; Pad < PadCount; Pad++)
        Banner.WriteU8(0);
}

void COpeningBanner::Save()
{
    TString BannerPath = mpProj->DiscFilesystemRoot(false) + "opening.bnr";
    CFileOutStream Banner(BannerPath, std::endian::big);
    Banner.WriteBytes(mBannerData.data(), mBannerData.size());
}

uint32_t COpeningBanner::MaxGameNameLength() const
{
    return (mWii ? 21 : 64);
}
