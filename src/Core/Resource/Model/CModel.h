#ifndef CMODEL_H
#define CMODEL_H

#include <Common/CColor.h>
#include "Core/OpenGL/GLCommon.h"
#include <Core/Resource/TResPtr.h>
#include "Core/Resource/Model/CBasicModel.h"
#include "Core/Render/FRenderOptions.h"

#include <memory>
#include <vector>

class CIndexBuffer;
class CMaterial;
class CMaterialSet;
class CSkin;

class CModel : public CBasicModel
{
    friend class CModelLoader;
    friend class CModelCooker;

    TResPtr<CSkin> mpSkin;
    std::vector<CMaterialSet*> mMaterialSets;
    std::vector<std::vector<CIndexBuffer>> mSurfaceIndexBuffers;
    bool mHasOwnMaterials;
    
public:
    explicit CModel(CResourceEntry *pEntry = nullptr);
    explicit CModel(CMaterialSet *pSet, bool OwnsMatSet);
    ~CModel() override;

    std::unique_ptr<CDependencyTree> BuildDependencyTree() override;
    void BufferGL();
    void GenerateMaterialShaders();
    void ClearGLBuffer() override;
    void Draw(FRenderOptions Options, size_t MatSet);
    void DrawSurface(FRenderOptions Options, size_t Surface, size_t MatSet);
    void DrawWireframe(FRenderOptions Options, CColor WireColor = CColor::White());
    void SetSkin(CSkin *pSkin);

    size_t GetMatSetCount() const;
    size_t GetMatCount() const;
    CMaterialSet* GetMatSet(size_t MatSet);
    CMaterial* GetMaterialByIndex(size_t MatSet, size_t Index);
    CMaterial* GetMaterialBySurface(size_t MatSet, size_t Surface);
    bool HasTransparency(size_t MatSet) const;
    bool IsSurfaceTransparent(size_t Surface, size_t MatSet) const;
    bool IsLightmapped() const;

    bool IsSkinned() const { return mpSkin != nullptr; }

private:
    CIndexBuffer* InternalGetIBO(size_t Surface, EPrimitiveType Primitive);
};

#endif // MODEL_H
