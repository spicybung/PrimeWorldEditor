#ifndef CAUDIOGROUPLOADER_H
#define CAUDIOGROUPLOADER_H

#include <memory>

class CAudioGroup;
class CAudioLookupTable;
class CResourceEntry;
class CStringList;
class IInputStream;

class CAudioGroupLoader
{
public:
    CAudioGroupLoader() = delete;

    static std::unique_ptr<CAudioGroup> LoadAGSC(IInputStream& rAGSC, CResourceEntry *pEntry);
    static std::unique_ptr<CAudioLookupTable> LoadATBL(IInputStream& rATBL, CResourceEntry *pEntry);
    static std::unique_ptr<CStringList> LoadSTLC(IInputStream& rSTLC, CResourceEntry *pEntry);
};

#endif // CAUDIOGROUPLOADER_H
