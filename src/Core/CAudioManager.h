#ifndef CAUDIOMANAGER
#define CAUDIOMANAGER

#include <Common/TString.h>
#include "Core/Resource/TResPtr.h"

#include <unordered_map>
#include <vector>

class CAudioGroup;
class CAudioLookupTable;
class CGameProject;
class CStringList;

struct SSoundInfo
{
    CAudioGroup* pAudioGroup{};
    TString Name;
    uint32_t SoundID{};
    uint16_t DefineID{};
};

class CAudioManager
{
    CGameProject *mpProject;

    std::vector<TResPtr<CAudioGroup>> mAudioGroups;
    TResPtr<CAudioLookupTable> mpAudioLookupTable;
    TResPtr<CStringList> mpSfxNameList;
    std::unordered_map<uint16_t, CAudioGroup*> mSfxIdMap;

public:
    explicit CAudioManager(CGameProject* pProj);
    ~CAudioManager();

    void LoadAssets();
    void ClearAssets();
    SSoundInfo GetSoundInfo(uint32_t SoundID) const;
    void LogSoundInfo(uint32_t SoundID) const;
};

#endif // CAUDIOMANAGER

