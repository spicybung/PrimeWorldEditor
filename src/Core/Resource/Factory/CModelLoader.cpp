#include "Core/Resource/Factory/CModelLoader.h"

#include <Common/Log.h>
#include <Common/FileIO/IInputStream.h>
#include <Common/Math/CVector2f.h>
#include <Common/Math/CVector3f.h>

#include "Core/Resource/CMaterialSet.h"
#include "Core/Resource/Factory/CMaterialLoader.h"
#include "Core/Resource/Factory/CSectionMgrIn.h"
#include "Core/Resource/Model/CBasicModel.h"
#include "Core/Resource/Model/CModel.h"
#include "Core/Resource/Model/SSurface.h"

#include <assimp/scene.h>
#include <map>

CModelLoader::CModelLoader() = default;

CModelLoader::~CModelLoader() = default;

void CModelLoader::LoadWorldMeshHeader(IInputStream& rModel)
{
    // I don't really have any need for most of this data, so
    rModel.Seek(0x34, SEEK_CUR);
    mAABox = CAABox(rModel);
    mpSectionMgr->ToNextSection();
}

void CModelLoader::LoadAttribArrays(IInputStream& rModel)
{
    // Positions
    if (mFlags.HasFlag(EModelLoaderFlag::HalfPrecisionPositions)) // 16-bit (DKCR only)
    {
        mPositions.resize(mpSectionMgr->CurrentSectionSize() / 0x6);
        constexpr float Divisor = 8192.f; // Might be incorrect! Needs verification via size comparison.

        for (auto& position : mPositions)
        {
            position.X = static_cast<float>(rModel.ReadS16()) / Divisor;
            position.Y = static_cast<float>(rModel.ReadS16()) / Divisor;
            position.Z = static_cast<float>(rModel.ReadS16()) / Divisor;
        }
    }
    else // 32-bit
    {
        mPositions.resize(mpSectionMgr->CurrentSectionSize() / 0xC);

        for (auto& position : mPositions)
            position = CVector3f(rModel);
    }

    mpSectionMgr->ToNextSection();

    // Normals
    if (mFlags.HasFlag(EModelLoaderFlag::HalfPrecisionNormals)) // 16-bit
    {
        mNormals.resize(mpSectionMgr->CurrentSectionSize() / 0x6);
        const float Divisor = (mVersion < EGame::DKCReturns) ? 32768.f : 16384.f;

        for (auto& normal : mNormals)
        {
            normal.X = static_cast<float>(rModel.ReadS16()) / Divisor;
            normal.Y = static_cast<float>(rModel.ReadS16()) / Divisor;
            normal.Z = static_cast<float>(rModel.ReadS16()) / Divisor;
        }
    }
    else // 32-bit
    {
        mNormals.resize(mpSectionMgr->CurrentSectionSize() / 0xC);

        for (auto& normal : mNormals)
            normal = CVector3f(rModel);
    }

    mpSectionMgr->ToNextSection();

    // Colors
    mColors.resize(mpSectionMgr->CurrentSectionSize() / 4);
    for (auto& color : mColors)
    {
        color = CColor(rModel);
    }
    mpSectionMgr->ToNextSection();


    // UVs
    mTex0.resize(mpSectionMgr->CurrentSectionSize() / 0x8);
    for (auto& vec : mTex0)
    {
        vec = CVector2f(rModel);
    }
    mpSectionMgr->ToNextSection();

    // Lightmap UVs
    if (mFlags.HasFlag(EModelLoaderFlag::LightmapUVs))
    {
        mTex1.resize(mpSectionMgr->CurrentSectionSize() / 0x4);
        const float Divisor = (mVersion < EGame::DKCReturns) ? 32768.f : 8192.f;

        for (auto& vec : mTex1)
        {
            vec.X = static_cast<float>(rModel.ReadS16()) / Divisor;
            vec.Y = static_cast<float>(rModel.ReadS16()) / Divisor;
        }

        mpSectionMgr->ToNextSection();
    }
}

void CModelLoader::LoadSurfaceOffsets(IInputStream& rModel)
{
    mSurfaceCount = rModel.ReadU32();
    mSurfaceOffsets.resize(mSurfaceCount);

    for (size_t iSurf = 0; iSurf < mSurfaceCount; iSurf++)
        mSurfaceOffsets[iSurf] = rModel.ReadU32();

    mpSectionMgr->ToNextSection();
}

SSurface* CModelLoader::LoadSurface(IInputStream& rModel)
{
    auto* pSurf = new SSurface();

    // Surface header
    if (mVersion  < EGame::DKCReturns)
        LoadSurfaceHeaderPrime(rModel, pSurf);
    else
        LoadSurfaceHeaderDKCR(rModel, pSurf);

    const bool HasAABB = pSurf->AABox != CAABox::Infinite();
    CMaterial *pMat = mMaterials[0]->MaterialByIndex(pSurf->MaterialID, false);

    // Primitive table
    uint8_t Flag = rModel.ReadU8();
    const uint32_t NextSurface = mpSectionMgr->NextOffset();

    while (Flag != 0 && (rModel.Tell() < NextSurface))
    {
        const auto VertexCount = rModel.ReadU16();

        SSurface::SPrimitive& Prim = pSurf->Primitives.emplace_back();
        Prim.Type = EPrimitiveType(Flag & 0xF8);
        Prim.Vertices.reserve(VertexCount);

        for (uint32_t iVtx = 0; iVtx < VertexCount; iVtx++)
        {
            const FVertexDescription VtxDesc = pMat->VtxDesc();
            CVertex& Vtx = Prim.Vertices.emplace_back();

            for (uint32_t iMtxAttr = 0; iMtxAttr < 8; iMtxAttr++)
            {
                if (VtxDesc.HasFlag(EVertexAttribute::PosMtx << iMtxAttr))
                    rModel.Seek(0x1, SEEK_CUR);
            }

            // Only thing to do here is check whether each attribute is present, and if so, read it.
            // A couple attributes have special considerations; normals can be floats or shorts, as can tex0, depending on vtxfmt.
            // tex0 can also be read from either UV buffer; depends what the material says.

            // Position
            if (VtxDesc.HasFlag(EVertexAttribute::Position))
            {
                const auto PosIndex = rModel.ReadU16();
                Vtx.Position = mPositions[PosIndex];
                Vtx.ArrayPosition = PosIndex;

                if (!HasAABB)
                    pSurf->AABox.ExpandBounds(Vtx.Position);
            }

            // Normal
            if (VtxDesc.HasFlag(EVertexAttribute::Normal))
                Vtx.Normal = mNormals[rModel.ReadU16()];

            // Color
            for (size_t iClr = 0; iClr < Vtx.Color.size(); iClr++)
            {
                if (VtxDesc.HasFlag(EVertexAttribute::Color0 << iClr))
                    Vtx.Color[iClr] = mColors[rModel.ReadU16()];
            }

            // Tex Coords - these are done a bit differently in DKCR than in the Prime series
            if (mVersion < EGame::DKCReturns)
            {
                // Tex0
                if (VtxDesc.HasFlag(EVertexAttribute::Tex0))
                {
                    if (mFlags.HasFlag(EModelLoaderFlag::LightmapUVs) && pMat->Options().HasFlag(EMaterialOption::ShortTexCoord))
                        Vtx.Tex[0] = mTex1[rModel.ReadU16()];
                    else
                        Vtx.Tex[0] = mTex0[rModel.ReadU16()];
                }

                // Tex1-7
                for (size_t iTex = 1; iTex < 7; iTex++)
                {
                    if (VtxDesc.HasFlag(EVertexAttribute::Tex0 << iTex))
                        Vtx.Tex[iTex] = mTex0[rModel.ReadU16()];
                }
            }
            else
            {
                // Tex0-7
                for (size_t iTex = 0; iTex < 7; iTex++)
                {
                    if (VtxDesc.HasFlag(EVertexAttribute::Tex0 << iTex))
                    {
                        if (!mSurfaceUsingTex1)
                            Vtx.Tex[iTex] = mTex0[rModel.ReadU16()];
                        else
                            Vtx.Tex[iTex] = mTex1[rModel.ReadU16()];
                    }
                }
            }
        } // Vertex array end

        // Update vertex/triangle count
        pSurf->VertexCount += VertexCount;

        switch (Prim.Type)
        {
            case EPrimitiveType::Triangles:
                pSurf->TriangleCount += VertexCount / 3;
                break;
            case EPrimitiveType::TriangleFan:
            case EPrimitiveType::TriangleStrip:
                pSurf->TriangleCount += VertexCount - 2;
                break;
            default:
                break;
        }

        Flag = rModel.ReadU8();
    } // Primitive table end

    mpSectionMgr->ToNextSection();
    return pSurf;
}

void CModelLoader::LoadSurfaceHeaderPrime(IInputStream& rModel, SSurface *pSurf)
{
    pSurf->CenterPoint = CVector3f(rModel);
    pSurf->MaterialID = rModel.ReadU32();

    rModel.Seek(0xC, SEEK_CUR);
    uint32_t ExtraSize = rModel.ReadU32();
    pSurf->ReflectionDirection = CVector3f(rModel);

    if (mVersion >= EGame::EchoesDemo)
        rModel.Seek(0x4, SEEK_CUR); // Skipping unknown values

    const bool HasAABox = (ExtraSize >= 0x18); // MREAs have a set of bounding box coordinates here.

    // If this surface has a bounding box, we can just read it here. Otherwise we'll fill it in manually.
    if (HasAABox)
    {
        ExtraSize -= 0x18;
        pSurf->AABox = CAABox(rModel);
    }
    else
    {
        pSurf->AABox = CAABox::Infinite();
    }

    rModel.Seek(ExtraSize, SEEK_CUR);
    rModel.SeekToBoundary(32);
}

void CModelLoader::LoadSurfaceHeaderDKCR(IInputStream& rModel, SSurface *pSurf)
{
    pSurf->CenterPoint = CVector3f(rModel);
    rModel.Seek(0xE, SEEK_CUR);
    pSurf->MaterialID = rModel.ReadU16();
    rModel.Seek(0x2, SEEK_CUR);
    mSurfaceUsingTex1 = rModel.ReadU8() == 1;
    uint32_t ExtraSize = rModel.ReadU8();

    if (ExtraSize > 0)
    {
        ExtraSize -= 0x18;
        pSurf->AABox = CAABox(rModel);
    }
    else
    {
        pSurf->AABox = CAABox::Infinite();
    }

    rModel.Seek(ExtraSize, SEEK_CUR);
    rModel.SeekToBoundary(32);
}

SSurface* CModelLoader::LoadAssimpMesh(const aiMesh *pkMesh, CMaterialSet *pSet)
{
    // Create vertex description and assign it to material
    CMaterial *pMat = pSet->MaterialByIndex(pkMesh->mMaterialIndex, false);
    FVertexDescription Desc = pMat->VtxDesc();

    if (Desc == EVertexAttribute::None)
    {
        if (pkMesh->HasPositions())
            Desc |= EVertexAttribute::Position;
        if (pkMesh->HasNormals())
            Desc |= EVertexAttribute::Normal;

        for (size_t iUV = 0; iUV < pkMesh->GetNumUVChannels(); iUV++)
            Desc |= EVertexAttribute::Tex0 << iUV;

        pMat->SetVertexDescription(Desc);

        // TEMP - disable dynamic lighting on geometry with no normals
        if (!pkMesh->HasNormals())
        {
            pMat->SetLightingEnabled(false);
            pMat->Pass(0)->SetColorInputs(kZeroRGB, kOneRGB, kKonstRGB, kZeroRGB);
            pMat->Pass(0)->SetRasSel(kRasColorNull);
        }
    }

    // Create surface
    auto* pSurf = new SSurface();
    pSurf->MaterialID = pkMesh->mMaterialIndex;

    if (pkMesh->mNumFaces > 0)
    {
        pSurf->Primitives.resize(1);
        SSurface::SPrimitive& rPrim = pSurf->Primitives[0];

        // Check primitive type on first face
        const uint32_t NumIndices = pkMesh->mFaces[0].mNumIndices;
        if (NumIndices == 1)
            rPrim.Type = EPrimitiveType::Points;
        else if (NumIndices == 2)
            rPrim.Type = EPrimitiveType::Lines;
        else if (NumIndices == 3)
            rPrim.Type = EPrimitiveType::Triangles;

        // Generate bounding box, center point, and reflection projection
        pSurf->CenterPoint = CVector3f::Zero();
        pSurf->ReflectionDirection = CVector3f::Zero();

        for (size_t iVtx = 0; iVtx < pkMesh->mNumVertices; iVtx++)
        {
            const aiVector3D AiPos = pkMesh->mVertices[iVtx];
            pSurf->AABox.ExpandBounds(CVector3f(AiPos.x, AiPos.y, AiPos.z));

            if (pkMesh->HasNormals()) {
                const aiVector3D aiNrm = pkMesh->mNormals[iVtx];
                pSurf->ReflectionDirection += CVector3f(aiNrm.x, aiNrm.y, aiNrm.z);
            }
        }
        pSurf->CenterPoint = pSurf->AABox.Center();

        if (pkMesh->HasNormals())
            pSurf->ReflectionDirection /= static_cast<float>(pkMesh->mNumVertices);
        else
            pSurf->ReflectionDirection = CVector3f(1.f, 0.f, 0.f);

        // Set vertex/triangle count
        pSurf->VertexCount = pkMesh->mNumVertices;
        pSurf->TriangleCount = (rPrim.Type == EPrimitiveType::Triangles ? pkMesh->mNumFaces : 0);

        // Create primitive
        for (size_t iFace = 0; iFace < pkMesh->mNumFaces; iFace++)
        {
            for (size_t iIndex = 0; iIndex < NumIndices; iIndex++)
            {
                const uint32_t Index = pkMesh->mFaces[iFace].mIndices[iIndex];

                // Create vertex and add it to the primitive
                CVertex& Vert = rPrim.Vertices.emplace_back();
                Vert.ArrayPosition = Index + mNumVertices;

                if (pkMesh->HasPositions())
                {
                    const aiVector3D AiPos = pkMesh->mVertices[Index];
                    Vert.Position = CVector3f(AiPos.x, AiPos.y, AiPos.z);
                }

                if (pkMesh->HasNormals())
                {
                    const aiVector3D AiNrm = pkMesh->mNormals[Index];
                    Vert.Normal = CVector3f(AiNrm.x, AiNrm.y, AiNrm.z);
                }

                for (size_t iTex = 0; iTex < pkMesh->GetNumUVChannels(); iTex++)
                {
                    const aiVector3D AiTex = pkMesh->mTextureCoords[iTex][Index];
                    Vert.Tex[iTex] = CVector2f(AiTex.x, AiTex.y);
                }
            }
        }

        mNumVertices += pkMesh->mNumVertices;
    }

    return pSurf;
}

// ************ STATIC ************
std::unique_ptr<CModel> CModelLoader::LoadCMDL(IInputStream& rCMDL, CResourceEntry *pEntry)
{
    CModelLoader Loader;

    // CMDL header - same across the three Primes, but different structure in DKCR
    const auto Magic = rCMDL.ReadU32();

    uint32_t Version, BlockCount, MatSetCount;
    CAABox AABox;

    // 0xDEADBABE - Metroid Prime seres
    if (Magic == 0xDEADBABE)
    {
        Version = rCMDL.ReadU32();
        const auto Flags = rCMDL.ReadU32();
        AABox = CAABox(rCMDL);
        BlockCount = rCMDL.ReadU32();
        MatSetCount = rCMDL.ReadU32();

        if ((Flags & 0x1) != 0)
            Loader.mFlags |= EModelLoaderFlag::Skinned;
        if ((Flags & 0x2) != 0)
            Loader.mFlags |= EModelLoaderFlag::HalfPrecisionNormals;
        if ((Flags & 0x4) != 0)
            Loader.mFlags |= EModelLoaderFlag::LightmapUVs;
    }
    // 0x9381000A - Donkey Kong Country Returns
    else if (Magic == 0x9381000A)
    {
        Version = Magic & 0xFFFF;
        const auto Flags = rCMDL.ReadU32();
        AABox = CAABox(rCMDL);
        BlockCount = rCMDL.ReadU32();
        MatSetCount = rCMDL.ReadU32();

        // todo: unknown flags
        Loader.mFlags = EModelLoaderFlag::HalfPrecisionNormals | EModelLoaderFlag::LightmapUVs;
        if ((Flags & 0x10) != 0)
            Loader.mFlags |= EModelLoaderFlag::VisibilityGroups;
        if ((Flags & 0x20) != 0)
            Loader.mFlags |= EModelLoaderFlag::HalfPrecisionPositions;

        // Visibility group data
        // Skipping for now - should read in eventually
        if ((Flags & 0x10) != 0)
        {
            rCMDL.Seek(0x4, SEEK_CUR);
            const auto VisGroupCount = rCMDL.ReadU32();

            for (uint32_t iVis = 0; iVis < VisGroupCount; iVis++)
            {
                const auto NameLength = rCMDL.ReadU32();
                rCMDL.Seek(NameLength, SEEK_CUR);
            }

            rCMDL.Seek(0x14, SEEK_CUR); // no clue what any of this is!
        }
    }
    else
    {
        NLog::Error("{}: Invalid CMDL magic: 0x{:08X}", rCMDL.GetSourceString(), Magic);
        return nullptr;
    }

    // The rest is common to all CMDL versions
    Loader.mVersion = GetFormatVersion(Version);

    if (Loader.mVersion == EGame::Invalid)
    {
        NLog::Error("{}: Unsupported CMDL version: 0x{:X}", rCMDL.GetSourceString(), Magic);
        return nullptr;
    }

    auto pModel = std::make_unique<CModel>(pEntry);
    Loader.mpModel = pModel.get();
    Loader.mpSectionMgr = new CSectionMgrIn(BlockCount, &rCMDL);
    rCMDL.SeekToBoundary(32);
    Loader.mpSectionMgr->Init();

    // Materials
    Loader.mMaterials.resize(MatSetCount);
    for (size_t iSet = 0; iSet < MatSetCount; iSet++)
    {
        Loader.mMaterials[iSet] = CMaterialLoader::LoadMaterialSet(rCMDL, Loader.mVersion, pEntry->ResourceStore());

        if (Loader.mVersion < EGame::CorruptionProto)
            Loader.mpSectionMgr->ToNextSection();
    }

    pModel->mMaterialSets = Loader.mMaterials;
    pModel->mHasOwnMaterials = true;
    if (Loader.mVersion >= EGame::CorruptionProto) Loader.mpSectionMgr->ToNextSection();

    // Mesh
    Loader.LoadAttribArrays(rCMDL);
    Loader.LoadSurfaceOffsets(rCMDL);
    pModel->mSurfaces.reserve(Loader.mSurfaceCount);

    for (size_t iSurf = 0; iSurf < Loader.mSurfaceCount; iSurf++)
    {
        SSurface *pSurf = Loader.LoadSurface(rCMDL);
        pModel->mSurfaces.push_back(pSurf);
        pModel->mVertexCount += pSurf->VertexCount;
        pModel->mTriangleCount += pSurf->TriangleCount;
    }

    pModel->mAABox = AABox;
    pModel->mHasOwnSurfaces = true;

    // Cleanup
    delete Loader.mpSectionMgr;
    return pModel;
}

std::unique_ptr<CModel> CModelLoader::LoadWorldModel(IInputStream& rMREA, CSectionMgrIn& rBlockMgr, CMaterialSet& rMatSet, EGame Version)
{
    CModelLoader Loader;
    Loader.mpSectionMgr = &rBlockMgr;
    Loader.mVersion = Version;
    Loader.mFlags = EModelLoaderFlag::HalfPrecisionNormals;
    if (Version != EGame::CorruptionProto)
        Loader.mFlags |= EModelLoaderFlag::LightmapUVs;
    Loader.mMaterials.resize(1);
    Loader.mMaterials[0] = &rMatSet;

    Loader.LoadWorldMeshHeader(rMREA);
    Loader.LoadAttribArrays(rMREA);
    Loader.LoadSurfaceOffsets(rMREA);

    auto pModel = std::make_unique<CModel>();
    pModel->mMaterialSets.resize(1);
    pModel->mMaterialSets[0] = &rMatSet;
    pModel->mHasOwnMaterials = false;
    pModel->mSurfaces.reserve(Loader.mSurfaceCount);
    pModel->mHasOwnSurfaces = true;

    for (size_t iSurf = 0; iSurf < Loader.mSurfaceCount; iSurf++)
    {
        SSurface *pSurf = Loader.LoadSurface(rMREA);
        pModel->mSurfaces.push_back(pSurf);
        pModel->mVertexCount += pSurf->VertexCount;
        pModel->mTriangleCount += pSurf->TriangleCount;
    }

    pModel->mAABox = Loader.mAABox;
    return pModel;
}

std::unique_ptr<CModel> CModelLoader::LoadCorruptionWorldModel(IInputStream& rMREA, CSectionMgrIn& rBlockMgr, CMaterialSet& rMatSet, uint32_t HeaderSecNum, uint32_t GPUSecNum, EGame Version)
{
    CModelLoader Loader;
    Loader.mpSectionMgr = &rBlockMgr;
    Loader.mVersion = Version;
    Loader.mFlags = EModelLoaderFlag::HalfPrecisionNormals;
    Loader.mMaterials.resize(1);
    Loader.mMaterials[0] = &rMatSet;
    if (Version == EGame::DKCReturns)
        Loader.mFlags |= EModelLoaderFlag::LightmapUVs;

    // Corruption/DKCR MREAs split the mesh header and surface offsets away from the actual geometry data so I need two section numbers to read it
    rBlockMgr.ToSection(HeaderSecNum);
    Loader.LoadWorldMeshHeader(rMREA);
    Loader.LoadSurfaceOffsets(rMREA);
    rBlockMgr.ToSection(GPUSecNum);
    Loader.LoadAttribArrays(rMREA);

    auto pModel = std::make_unique<CModel>();
    pModel->mMaterialSets.resize(1);
    pModel->mMaterialSets[0] = &rMatSet;
    pModel->mHasOwnMaterials = false;
    pModel->mSurfaces.reserve(Loader.mSurfaceCount);
    pModel->mHasOwnSurfaces = true;

    for (size_t iSurf = 0; iSurf < Loader.mSurfaceCount; iSurf++)
    {
        SSurface *pSurf = Loader.LoadSurface(rMREA);
        pModel->mSurfaces.push_back(pSurf);
        pModel->mVertexCount += pSurf->VertexCount;
        pModel->mTriangleCount += pSurf->TriangleCount;
    }

    pModel->mAABox = Loader.mAABox;
    return pModel;
}

void CModelLoader::BuildWorldMeshes(std::vector<std::unique_ptr<CModel>>& rkIn, std::vector<std::unique_ptr<CModel>>& rOut, bool DeleteInputModels)
{
    // This function takes the gigantic models with all surfaces combined from MP2/3/DKCR and splits the surfaces to reform the original uncombined meshes.
    std::map<uint32_t, CModel*> OutputMap;

    for (auto& pModel : rkIn)
    {
        pModel->mHasOwnSurfaces = false;
        pModel->mHasOwnMaterials = false;

        for (SSurface* pSurf : pModel->mSurfaces)
        {
            const auto ID = static_cast<uint32_t>(pSurf->MeshID);
            const auto Iter = OutputMap.find(ID);

            // No model for this ID; create one!
            if (Iter == OutputMap.cend())
            {
                auto pOutMdl = std::make_unique<CModel>();
                pOutMdl->mMaterialSets.resize(1);
                pOutMdl->mMaterialSets[0] = pModel->mMaterialSets[0];
                pOutMdl->mHasOwnMaterials = false;
                pOutMdl->mSurfaces.push_back(pSurf);
                pOutMdl->mHasOwnSurfaces = true;
                pOutMdl->mVertexCount = pSurf->VertexCount;
                pOutMdl->mTriangleCount = pSurf->TriangleCount;
                pOutMdl->mAABox.ExpandBounds(pSurf->AABox);

                OutputMap.insert_or_assign(ID, pOutMdl.get());
                rOut.push_back(std::move(pOutMdl));
            }
            else // Existing model; add this surface to it
            {
                CModel *pOutMdl = Iter->second;
                pOutMdl->mSurfaces.push_back(pSurf);
                pOutMdl->mVertexCount += pSurf->VertexCount;
                pOutMdl->mTriangleCount += pSurf->TriangleCount;
                pOutMdl->mAABox.ExpandBounds(pSurf->AABox);
            }
        }

        // Done with this model, should we delete it?
        if (DeleteInputModels)
            pModel.reset();
    }
}

CModel* CModelLoader::ImportAssimpNode(const aiNode *pkNode, const aiScene *pkScene, CMaterialSet& rMatSet)
{
    CModelLoader Loader;
    Loader.mpModel = new CModel(&rMatSet, true);
    Loader.mpModel->mSurfaces.reserve(pkNode->mNumMeshes);

    for (size_t iMesh = 0; iMesh < pkNode->mNumMeshes; iMesh++)
    {
        const uint32_t MeshIndex = pkNode->mMeshes[iMesh];
        const aiMesh *pkMesh = pkScene->mMeshes[MeshIndex];
        SSurface *pSurf = Loader.LoadAssimpMesh(pkMesh, &rMatSet);

        Loader.mpModel->mSurfaces.push_back(pSurf);
        Loader.mpModel->mAABox.ExpandBounds(pSurf->AABox);
        Loader.mpModel->mVertexCount += pSurf->VertexCount;
        Loader.mpModel->mTriangleCount += pSurf->TriangleCount;
    }

    return Loader.mpModel;
}

EGame CModelLoader::GetFormatVersion(uint32_t Version)
{
    switch (Version)
    {
        case 0x2: return EGame::Prime;
        case 0x3: return EGame::EchoesDemo;
        case 0x4: return EGame::Echoes;
        case 0x5: return EGame::Corruption;
        case 0xA: return EGame::DKCReturns;
        default: return EGame::Invalid;
    }
}
