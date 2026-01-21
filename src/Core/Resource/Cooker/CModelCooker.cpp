#include "Core/Resource/Cooker/CModelCooker.h"

#include "Core/Resource/Cooker/CMaterialCooker.h"
#include "Core/Resource/Cooker/CSectionMgrOut.h"
#include "Core/Resource/CMaterial.h"
#include "Core/Resource/Model/CModel.h"
#include "Core/Resource/Model/SSurface.h"

#include <algorithm>

CModelCooker::CModelCooker() = default;
CModelCooker::~CModelCooker() = default;

void CModelCooker::GenerateSurfaceData()
{
    // Need to gather metadata from the model before we can start
    mNumMatSets = mpModel->mMaterialSets.size();
    mNumSurfaces = mpModel->mSurfaces.size();
    mNumVertices = mpModel->mVertexCount;
    mVertices.resize(mNumVertices);

    // Get vertex attributes
    mVtxAttribs = EVertexAttribute::None;

    for (size_t iMat = 0; iMat < mpModel->GetMatCount(); iMat++)
    {
        CMaterial *pMat = mpModel->GetMaterialByIndex(0, iMat);
        mVtxAttribs |= pMat->VtxDesc();
    }

    // Get vertices
    size_t MaxIndex = 0;

    for (size_t iSurf = 0; iSurf < mNumSurfaces; iSurf++)
    {
        const size_t NumPrimitives = mpModel->mSurfaces[iSurf]->Primitives.size();

        for (size_t iPrim = 0; iPrim < NumPrimitives; iPrim++)
        {
            const SSurface::SPrimitive *pPrim = &mpModel->mSurfaces[iSurf]->Primitives[iPrim];
            const size_t NumVerts = pPrim->Vertices.size();

            for (size_t iVtx = 0; iVtx < NumVerts; iVtx++)
            {
                const uint32_t VertIndex = pPrim->Vertices[iVtx].ArrayPosition;
                mVertices[VertIndex] = pPrim->Vertices[iVtx];

                if (VertIndex > MaxIndex)
                    MaxIndex = VertIndex;
            }
        }
    }

    mVertices.resize(MaxIndex + 1);
    mNumVertices = mVertices.size();
}

void CModelCooker::WriteEditorModel(IOutputStream& /*rOut*/)
{
}

void CModelCooker::WriteModelPrime(IOutputStream& rOut)
{
    GenerateSurfaceData();

    // Header
    rOut.WriteU32(0xDEADBABE);
    rOut.WriteU32(GetCMDLVersion(mVersion));
    rOut.WriteU32(mpModel->IsSkinned() ? 6 : 5);
    mpModel->mAABox.Write(rOut);

    const uint32_t NumSections = mNumMatSets + mNumSurfaces + 6;
    rOut.WriteU32(NumSections);
    rOut.WriteU32(mNumMatSets);

    const uint32_t SectionSizesOffset = rOut.Tell();
    for (uint32_t iSec = 0; iSec < NumSections; iSec++)
        rOut.WriteU32(0);

    rOut.WriteToBoundary(32, 0);

    std::vector<uint32_t> SectionSizes;
    SectionSizes.reserve(NumSections);

    CSectionMgrOut SectionMgr;
    SectionMgr.SetSectionCount(NumSections);
    SectionMgr.Init(rOut);

    // Materials
    for (size_t iSet = 0; iSet < mNumMatSets; iSet++)
    {
        CMaterialCooker::WriteCookedMatSet(mpModel->mMaterialSets[iSet], mVersion, rOut);
        rOut.WriteToBoundary(32, 0);
        SectionMgr.AddSize(rOut);
    }

    // Vertices
    for (size_t iPos = 0; iPos < mNumVertices; iPos++)
        mVertices[iPos].Position.Write(rOut);

    rOut.WriteToBoundary(32, 0);
    SectionMgr.AddSize(rOut);

    // Normals
    for (size_t iNrm = 0; iNrm < mNumVertices; iNrm++)
        mVertices[iNrm].Normal.Write(rOut);

    rOut.WriteToBoundary(32, 0);
    SectionMgr.AddSize(rOut);

    // Colors
    for (size_t iColor = 0; iColor < mNumVertices; iColor++)
        mVertices[iColor].Color[0].Write(rOut);

    rOut.WriteToBoundary(32, 0);
    SectionMgr.AddSize(rOut);

    // Float UV coordinates
    for (size_t iTexSlot = 0; iTexSlot < 8; iTexSlot++)
    {
        const auto TexSlotBit = EVertexAttribute(EVertexAttribute::Tex0 << iTexSlot);
        const bool HasTexSlot = mVtxAttribs.HasFlag(TexSlotBit);
        if (HasTexSlot)
        {
            for (size_t iTex = 0; iTex < mNumVertices; iTex++)
                mVertices[iTex].Tex[iTexSlot].Write(rOut);
        }
    }

    rOut.WriteToBoundary(32, 0);
    SectionMgr.AddSize(rOut);
    SectionMgr.AddSize(rOut); // Skipping short UV coordinates

    // Surface offsets
    rOut.WriteU32(mNumSurfaces);
    const uint32_t SurfaceOffsetsStart = rOut.Tell();

    for (uint32_t iSurf = 0; iSurf < mNumSurfaces; iSurf++)
        rOut.WriteU32(0);

    rOut.WriteToBoundary(32, 0);
    SectionMgr.AddSize(rOut);

    // Surfaces
    const uint32_t SurfacesStart = rOut.Tell();
    std::vector<uint32_t> SurfaceEndOffsets(mNumSurfaces);

    for (size_t iSurf = 0; iSurf < mNumSurfaces; iSurf++)
    {
        SSurface *pSurface = mpModel->GetSurface(iSurf);

        pSurface->CenterPoint.Write(rOut);
        rOut.WriteU32(pSurface->MaterialID);
        rOut.WriteU16(0x8000);
        const uint32_t PrimTableSizeOffset = rOut.Tell();
        rOut.WriteU16(0);
        rOut.WriteU64(0);
        rOut.WriteU32(0);
        pSurface->ReflectionDirection.Write(rOut);
        rOut.WriteToBoundary(32, 0);

        const uint32_t PrimTableStart = rOut.Tell();
        const FVertexDescription VtxAttribs = mpModel->GetMaterialBySurface(0, iSurf)->VtxDesc();

        for (const SSurface::SPrimitive& pPrimitive : pSurface->Primitives)
        {
            rOut.WriteU8(static_cast<uint8_t>(pPrimitive.Type));
            rOut.WriteU16(static_cast<uint16_t>(pPrimitive.Vertices.size()));

            for (const CVertex& pVert : pPrimitive.Vertices)
            {
                if (mVersion == EGame::Echoes)
                {
                    for (size_t iMtxAttribs = 0; iMtxAttribs < pVert.MatrixIndices.size(); iMtxAttribs++)
                    {
                        const auto MatrixBit = EVertexAttribute(EVertexAttribute::PosMtx << iMtxAttribs);
                        if (VtxAttribs.HasFlag(MatrixBit))
                        {
                            rOut.WriteU8(pVert.MatrixIndices[iMtxAttribs]);
                        }
                    }
                }

                const auto VertexIndex = static_cast<uint16_t>(pVert.ArrayPosition);

                if (VtxAttribs.HasFlag(EVertexAttribute::Position))
                    rOut.WriteU16(VertexIndex);

                if (VtxAttribs.HasFlag(EVertexAttribute::Normal))
                    rOut.WriteU16(VertexIndex);

                if (VtxAttribs.HasFlag(EVertexAttribute::Color0))
                    rOut.WriteU16(VertexIndex);

                if (VtxAttribs.HasFlag(EVertexAttribute::Color1))
                    rOut.WriteU16(VertexIndex);

                uint16_t TexOffset = 0;
                for (uint32_t iTex = 0; iTex < 8; iTex++)
                {
                    const auto TexBit = EVertexAttribute(EVertexAttribute::Tex0 << iTex);

                    if (VtxAttribs.HasFlag(TexBit))
                    {
                        rOut.WriteU16(static_cast<uint16_t>(VertexIndex + TexOffset));
                        TexOffset += static_cast<uint16_t>(mNumVertices);
                    }
                }
            }
        }

        rOut.WriteToBoundary(32, 0);
        const uint32_t PrimTableEnd = rOut.Tell();
        const uint32_t PrimTableSize = PrimTableEnd - PrimTableStart;
        rOut.Seek(PrimTableSizeOffset, SEEK_SET);
        rOut.WriteU16(static_cast<uint16_t>(PrimTableSize));
        rOut.Seek(PrimTableEnd, SEEK_SET);

        SectionMgr.AddSize(rOut);
        SurfaceEndOffsets[iSurf] = rOut.Tell() - SurfacesStart;
    }

    // Done writing the file - now we go back to fill in surface offsets + section sizes
    rOut.Seek(SurfaceOffsetsStart, SEEK_SET);

    for (size_t iSurf = 0; iSurf < mNumSurfaces; iSurf++)
        rOut.WriteU32(SurfaceEndOffsets[iSurf]);

    rOut.Seek(SectionSizesOffset, SEEK_SET);
    SectionMgr.WriteSizes(rOut);

    // Done!
}

bool CModelCooker::CookCMDL(CModel *pModel, IOutputStream& rOut)
{
    CModelCooker Cooker;
    Cooker.mpModel = pModel;
    Cooker.mVersion = pModel->Game();

    switch (pModel->Game())
    {
    case EGame::PrimeDemo:
    case EGame::Prime:
    case EGame::EchoesDemo:
    case EGame::Echoes:
        Cooker.WriteModelPrime(rOut);
        return true;

    default:
        return false;
    }
}

uint32_t CModelCooker::GetCMDLVersion(EGame Version)
{
    switch (Version)
    {
    case EGame::PrimeDemo:
    case EGame::Prime:
        return 0x2;
    case EGame::EchoesDemo:
        return 0x3;
    case EGame::Echoes:
        return 0x4;
    case EGame::CorruptionProto:
    case EGame::Corruption:
        return 0x5;
    case EGame::DKCReturns:
        return 0xA;
    default:
        return 0;
    }
}
