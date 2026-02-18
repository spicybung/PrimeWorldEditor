#ifndef CFONTLOADER_H
#define CFONTLOADER_H

#include <Common/EGame.h>
#include <Core/Resource/TResPtr.h>

#include <memory>

class CFont;
class CResourceEntry;
class IInputStream;

class CFontLoader
{
    TResPtr<CFont> mpFont;
    EGame mVersion{};

    CFontLoader();
    void LoadFontImpl(IInputStream& rFONT);

public:
    static std::unique_ptr<CFont> LoadFONT(IInputStream& rFONT, CResourceEntry *pEntry);
    static EGame GetFormatVersion(uint32_t Version);
};

#endif // CFONTLOADER_H
