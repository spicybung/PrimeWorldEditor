#ifndef CDEPENDENCYGROUPLOADER_H
#define CDEPENDENCYGROUPLOADER_H

#include <Common/EGame.h>
#include <memory>

class CDependencyGroup;
class CResourceEntry;
class IInputStream;

class CDependencyGroupLoader
{
public:
    CDependencyGroupLoader() = delete;
    static std::unique_ptr<CDependencyGroup> LoadDGRP(IInputStream& rDGRP, CResourceEntry *pEntry);
};

#endif // CDEPENDENCYGROUPLOADER_H
