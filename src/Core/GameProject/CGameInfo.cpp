#include "Core/GameProject/CGameInfo.h"

#include "Core/GameProject/CResourceStore.h"
#include <Common/Macros.h>
#include <Common/Serialization/CXMLReader.h>
#include <Common/Serialization/CXMLWriter.h>
#include <Common/Serialization/IArchive.h>

#include <algorithm>
#include <fmt/format.h>

constexpr char gkGameInfoDir[] = "resources/gameinfo";
constexpr char gkGameInfoExt[] = "xml";

struct CGameInfo::SBuildInfo
{
    float Version;
    ERegion Region;
    TString Name;

    void Serialize(IArchive& rArc)
    {
        rArc << SerialParameter("Version", Version)
             << SerialParameter("Region", Region)
             << SerialParameter("Name", Name);
    }
};

CGameInfo::CGameInfo() = default;
CGameInfo::~CGameInfo() = default;

bool CGameInfo::LoadGameInfo(EGame Game)
{
    Game = RoundGame(Game);
    mGame = Game;

    TString Path = GetDefaultGameInfoPath(Game);
    return LoadGameInfo(Path);
}

bool CGameInfo::LoadGameInfo(const TString& Path)
{
    CXMLReader Reader(Path);

    if (Reader.IsValid())
    {
        Serialize(Reader);
        return true;
    }

    return false;
}

bool CGameInfo::SaveGameInfo(TString Path)
{
    ASSERT(mGame != EGame::Invalid); // can't save game info that was never loaded

    if (Path.IsEmpty())
        Path = GetDefaultGameInfoPath(mGame);

    CXMLWriter Writer(Path, "GameInfo", 0, mGame);
    Serialize(Writer);
    return Writer.Save();
}

void CGameInfo::Serialize(IArchive& rArc)
{
    // Validate game
    if (rArc.IsReader() && mGame != EGame::Invalid)
    {
        ASSERT(mGame == rArc.Game());
    }

    // Serialize data
    rArc << SerialParameter("GameBuilds", mBuilds);

    if (mGame <= EGame::Prime)
        rArc << SerialParameter("AreaNameMap", mAreaNameMap);
}

TString CGameInfo::GetBuildName(float BuildVer, ERegion Region) const
{
    const auto it = std::ranges::find_if(mBuilds,
                                         [=](const auto& entry) { return entry.Version == BuildVer && entry.Region == Region; });

    if (it == mBuilds.cend())
        return "Unknown Build";

    return it->Name;
}

TString CGameInfo::GetAreaName(const CAssetID &rkID) const
{
    const auto Iter = mAreaNameMap.find(rkID);
    return Iter == mAreaNameMap.cend() ? "" : Iter->second;
}

// ************ STATIC ************
EGame CGameInfo::RoundGame(EGame Game)
{
    if (Game == EGame::PrimeDemo)         return EGame::Prime;
    if (Game == EGame::EchoesDemo)        return EGame::Echoes;
    if (Game == EGame::CorruptionProto)   return EGame::Corruption;
    return Game;
}

TString CGameInfo::GetDefaultGameInfoPath(EGame Game)
{
    Game = RoundGame(Game);

    if (Game == EGame::Invalid)
        return "";

    const TString GameName = GetGameShortName(Game);
    return fmt::format("{}/{}/GameInfo{}.{}", gDataDir.ToStdString(), gkGameInfoDir, GameName.ToStdString(), gkGameInfoExt);
}

TString CGameInfo::GetExtension()
{
    return gkGameInfoExt;
}
