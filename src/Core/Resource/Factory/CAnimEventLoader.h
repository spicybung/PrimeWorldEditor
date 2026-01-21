#ifndef CANIMEVENTLOADER_H
#define CANIMEVENTLOADER_H

#include "Core/Resource/TResPtr.h"
#include <memory>

class CAnimEventData;

class CAnimEventLoader
{
    TResPtr<CAnimEventData> mpEventData;
    EGame mGame{};

    CAnimEventLoader() = default;
    void LoadEvents(IInputStream& rEVNT);
    int LoadEventBase(IInputStream& rEVNT);
    void LoadLoopEvent(IInputStream& rEVNT);
    void LoadUserEvent(IInputStream& rEVNT);
    void LoadEffectEvent(IInputStream& rEVNT);
    void LoadSoundEvent(IInputStream& rEVNT);

public:
    static std::unique_ptr<CAnimEventData> LoadEVNT(IInputStream& rEVNT, CResourceEntry *pEntry);
    static std::unique_ptr<CAnimEventData> LoadAnimSetEvents(IInputStream& rANCS);
    static std::unique_ptr<CAnimEventData> LoadCorruptionCharacterEventSet(IInputStream& rCHAR);
};

#endif // CANIMEVENTLOADER_H
