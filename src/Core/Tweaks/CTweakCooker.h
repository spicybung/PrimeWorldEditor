#ifndef CTWEAKCOOKER_H
#define CTWEAKCOOKER_H

#include <span>

class CTweakData;
class IOutputStream;

/** Class responsible for cooking tweak data */
class CTweakCooker
{
public:
    CTweakCooker() = delete;

    /** Cooker entry point */
    static bool CookCTWK(CTweakData* pTweakData, IOutputStream& CTWK);
    static bool CookNTWK(std::span<CTweakData*> tweaks, IOutputStream& NTWK);
};

#endif // CTWEAKCOOKER_H
