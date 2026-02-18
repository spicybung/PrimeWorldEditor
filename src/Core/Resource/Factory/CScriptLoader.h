#ifndef CSCRIPTLOADER_H
#define CSCRIPTLOADER_H

#include <Common/EGame.h>
#include "Core/Resource/Script/Property/TPropertyRef.h"

#include <cstdint>
#include <memory>

class CGameArea;
class CGameTemplate;
class CResourceStore;
class CScriptLayer;
class CScriptObject;
class CStructProperty;
class IInputStream;
class IProperty;

class CScriptLoader
{
    EGame mVersion{};
    CScriptObject* mpObj = nullptr;
    CScriptLayer* mpLayer = nullptr;
    CGameArea* mpArea = nullptr;
    CGameTemplate *mpGameTemplate = nullptr;
    CResourceStore* mResourceStore = nullptr;

    // Current data pointer
    void* mpCurrentData = nullptr;

    CScriptLoader();
    void ReadProperty(IProperty* pProp, uint32_t Size, IInputStream& rSCLY);

    void LoadStructMP1(IInputStream& rSCLY, CStructProperty* pStruct);
    CScriptObject* LoadObjectMP1(IInputStream& rSCLY);
    std::unique_ptr<CScriptLayer> LoadLayerMP1(IInputStream& rSCLY);

    void LoadStructMP2(IInputStream& rSCLY, CStructProperty* pStruct);
    CScriptObject* LoadObjectMP2(IInputStream& rSCLY);
    std::unique_ptr<CScriptLayer> LoadLayerMP2(IInputStream& rSCLY);

public:
    static std::unique_ptr<CScriptLayer> LoadLayer(IInputStream& rSCLY, CGameArea *pArea, EGame Version);
    static CScriptObject* LoadInstance(IInputStream& rSCLY, CGameArea *pArea, CScriptLayer *pLayer, EGame Version, bool ForceReturnsFormat);
    static void LoadStructData(IInputStream& rInput, CStructRef InStruct, CResourceStore* resourceStore);
};

#endif // CSCRIPTLOADER_H
