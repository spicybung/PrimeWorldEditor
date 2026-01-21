#ifndef CPOITOWORLDLOADER_H
#define CPOITOWORLDLOADER_H

#include <memory>

class CPoiToWorld;
class CResourceEntry;
class IInputStream;

class CPoiToWorldLoader
{
public:
    CPoiToWorldLoader() = delete;
    static std::unique_ptr<CPoiToWorld> LoadEGMC(IInputStream& rEGMC, CResourceEntry *pEntry);
};

#endif // CPOITOWORLDLOADER_H
