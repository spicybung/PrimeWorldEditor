#ifndef CTWEAKLOADER_H
#define CTWEAKLOADER_H

#include <Common/EGame.h>
#include <memory>
#include <vector>

class CResourceEntry;
class CResourceStore;
class CTweakData;
class IInputStream;

/** Class responsible for loading tweak data */
class CTweakLoader
{
public:
    CTweakLoader() = delete;

    /** Loader entry point */
    static std::unique_ptr<CTweakData> LoadCTWK(IInputStream& CTWK, CResourceEntry* pEntry);
    static void LoadNTWK(IInputStream& NTWK, EGame Game, std::vector<CTweakData*>& OutTweaks, CResourceStore* resourceStore);
};

#endif // CTWEAKLOADER_H
