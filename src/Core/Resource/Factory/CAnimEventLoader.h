#ifndef CANIMEVENTLOADER_H
#define CANIMEVENTLOADER_H

#include "Core/Resource/TResPtr.h"
#include <memory>

class CAnimEventData;
class CResourceStore;

class CAnimEventLoader
{
    TResPtr<CAnimEventData> mpEventData;
    EGame mGame{};
    CResourceStore* mResourceStore{};

    CAnimEventLoader() = default;
    void LoadEvents(IInputStream& rEVNT);
    int LoadEventBase(IInputStream& rEVNT);
    void LoadLoopEvent(IInputStream& rEVNT);
    void LoadUserEvent(IInputStream& rEVNT);
    void LoadEffectEvent(IInputStream& rEVNT);
    void LoadSoundEvent(IInputStream& rEVNT);

public:
    static std::unique_ptr<CAnimEventData> LoadEVNT(IInputStream& rEVNT, CResourceEntry *pEntry);
    static std::unique_ptr<CAnimEventData> LoadAnimSetEvents(IInputStream& rANCS, CResourceStore* resourceStore);
    static std::unique_ptr<CAnimEventData> LoadCorruptionCharacterEventSet(IInputStream& rCHAR, CResourceStore* resourceStore);
};

#endif // CANIMEVENTLOADER_H
