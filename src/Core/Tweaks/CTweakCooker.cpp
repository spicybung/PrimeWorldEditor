#include "Core/Tweaks/CTweakCooker.h"

#include <Common/CFourCC.h>
#include "Core/NRangeUtils.h"
#include "Core/Resource/Cooker/CScriptCooker.h"
#include "Core/Tweaks/CTweakData.h"

/** Cooker entry point */
bool CTweakCooker::CookCTWK(CTweakData* pTweakData, IOutputStream& CTWK)
{
    CStructRef TweakProperties = pTweakData->TweakData();
    CScriptCooker ScriptCooker(pTweakData->Game());
    ScriptCooker.WriteProperty(CTWK, TweakProperties.Property(), TweakProperties.DataPointer(), true);
    return true;
}

bool CTweakCooker::CookNTWK(std::span<CTweakData*> tweaks, IOutputStream& NTWK)
{
    NTWK.WriteFourCC(CFourCC("NTWK"));                    // NTWK magic
    NTWK.WriteU8(1);                                      // Version number; must be 1
    NTWK.WriteU32(static_cast<uint32_t>(tweaks.size())); // Number of tweak objects

    for (const auto [idx, tweakData] : Utils::enumerate(tweaks))
    {
        // Tweaks in MP2+ are saved with the script object data format
        // Write a dummy script object header here
        const uint32_t TweakObjectStart = NTWK.Tell();
        NTWK.WriteU32(tweakData->TweakID());     // Object ID
        NTWK.WriteU16(0);                        // Object size
        NTWK.WriteU32(uint32_t(idx));            // Instance ID
        NTWK.WriteU16(0);                        // Link count

        const CStructRef TweakProperties = tweakData->TweakData();
        CScriptCooker ScriptCooker(TweakProperties.Property()->Game());
        ScriptCooker.WriteProperty(NTWK, TweakProperties.Property(), TweakProperties.DataPointer(), false);

        const auto TweakObjectEnd = NTWK.Tell();
        const auto TweakObjectSize = static_cast<uint16_t>(TweakObjectEnd - TweakObjectStart - 6);
        NTWK.GoTo(TweakObjectStart + 4);
        NTWK.WriteU16(TweakObjectSize);
        NTWK.GoTo(TweakObjectEnd);
    }

    NTWK.WriteToBoundary(32, 0);
    return true;
}
