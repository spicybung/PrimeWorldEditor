#include "CCollisionRenderData.h"
#include <Core/Render/CDrawUtil.h>

#include <algorithm>
#include <array>
#include <numeric>
#include <vector>

/** Build from collision data */
void CCollisionRenderData::BuildRenderData(const SCollisionIndexData& kIndexData)
{
    // Clear any existing data
    if (mBuilt)
    {
        mVertexBuffer.Clear();
        mIndexBuffer.Clear();
        mWireframeIndexBuffer.Clear();
        mMaterialIndexOffsets.clear();
        mMaterialWireIndexOffsets.clear();
        mBuilt = false;
    }

    mIndexBuffer.SetPrimitiveType(GL_TRIANGLES);
    mWireframeIndexBuffer.SetPrimitiveType(GL_LINES);

    // Build list of triangle indices sorted by material index
    // Apparently some collision meshes have more triangle indices than actual triangles
    const auto NumTris = std::min(kIndexData.TriangleIndices.size() / 3, kIndexData.TriangleMaterialIndices.size());
    std::vector<uint16_t> SortedTris(NumTris);
    std::ranges::iota(SortedTris, uint16_t{0});
    std::ranges::sort(SortedTris, [kIndexData](uint16_t Left, uint16_t Right) -> bool {
        return kIndexData.TriangleMaterialIndices[Left] < kIndexData.TriangleMaterialIndices[Right];
    });

    mVertexBuffer.Reserve(SortedTris.size() * 3);
    mIndexBuffer.Reserve(SortedTris.size() * 3);
    mWireframeIndexBuffer.Reserve(SortedTris.size() * 6);
    mMaterialIndexOffsets.reserve(kIndexData.Materials.size());
    uint8_t CurrentMatIdx = 0xFF;

    for (const size_t TriIdx : SortedTris)
    {
        const uint8_t MaterialIdx = kIndexData.TriangleMaterialIndices[TriIdx];
        const CCollisionMaterial& kMaterial = kIndexData.Materials[MaterialIdx];

        if (MaterialIdx != CurrentMatIdx)
        {
            // Note some collision materials have no geometry associated with them as
            // some materials are exclusively used with edges/vertices.
            ASSERT(CurrentMatIdx < MaterialIdx || CurrentMatIdx == 0xFF);

            while (CurrentMatIdx != MaterialIdx)
            {
                mMaterialIndexOffsets.push_back(mIndexBuffer.GetSize());
                CurrentMatIdx++;
            }
        }

        const size_t LineA = kIndexData.TriangleIndices[(TriIdx * 3) + 0];
        const size_t LineB = kIndexData.TriangleIndices[(TriIdx * 3) + 1];
        const uint16_t LineAVertA = kIndexData.EdgeIndices[(LineA * 2) + 0];
        const uint16_t LineAVertB = kIndexData.EdgeIndices[(LineA * 2) + 1];
        const uint16_t LineBVertA = kIndexData.EdgeIndices[(LineB * 2) + 0];
        const uint16_t LineBVertB = kIndexData.EdgeIndices[(LineB * 2) + 1];
        uint16_t VertIdx0 = LineAVertA;
        uint16_t VertIdx1 = LineAVertB;
        uint16_t VertIdx2 = (LineBVertA != LineAVertA && LineBVertA != LineAVertB ? LineBVertA : LineBVertB);

        // Reverse vertex order if material indicates tri is flipped
        if (kMaterial.HasFlag(eCF_FlippedTri))
        {
            std::swap(VertIdx0, VertIdx2);
        }

        // Generate vertex data
        const CVector3f& kVert0 = kIndexData.Vertices[VertIdx0];
        const CVector3f& kVert1 = kIndexData.Vertices[VertIdx1];
        const CVector3f& kVert2 = kIndexData.Vertices[VertIdx2];
        const CVector3f V0toV1 = (kVert1 - kVert0);
        const CVector3f V0toV2 = (kVert2 - kVert0);
        const CVector3f TriNormal = V0toV1.Cross(V0toV2).Normalized();
        const uint16_t Index0 = static_cast<uint16_t>(mVertexBuffer.Size());
        const uint16_t Index1 = Index0 + 1;
        const uint16_t Index2 = Index1 + 1;

        CVertex Vtx;
        Vtx.Normal = TriNormal;

        Vtx.Position = kVert0;
        mVertexBuffer.AddVertex(Vtx);
        Vtx.Position = kVert1;
        mVertexBuffer.AddVertex(Vtx);
        Vtx.Position = kVert2;
        mVertexBuffer.AddVertex(Vtx);

        mIndexBuffer.AddIndices({Index0, Index1, Index2});

        mWireframeIndexBuffer.AddIndices({Index0, Index1, Index1, Index2, Index2, Index0});
    }

    // Fill the rest of the material offsets, adding an extra index at the end
    for (; CurrentMatIdx <= kIndexData.Materials.size(); CurrentMatIdx++)
    {
        mMaterialIndexOffsets.push_back(mIndexBuffer.GetSize());
    }

    // Done
    mVertexBuffer.Buffer();
    mIndexBuffer.Buffer();
    mWireframeIndexBuffer.Buffer();
    mBuilt = true;
}

void CCollisionRenderData::BuildBoundingHierarchyRenderData(const SOBBTreeNode* pOBBTree)
{
    if (mBoundingHierarchyBuilt)
    {
        mBoundingVertexBuffer.Clear();
        mBoundingIndexBuffer.Clear();
        mBoundingDepthOffsets.clear();
        mBoundingHierarchyBuilt = false;
    }

    mBoundingIndexBuffer.SetPrimitiveType(GL_LINES);

    // Iterate through the OBB tree, building a list of nodes as we go.
    // We iterate through this using a breadth-first traversal in order to group together
    // OBBs in the same depth level in the index buffer. This allows us to render a
    // subset of the bounding hierarchy based on a max depth level.
    std::vector<const SOBBTreeNode*> TreeNodes;
    TreeNodes.push_back(pOBBTree);
    size_t NodeIdx = 0;

    while (NodeIdx < TreeNodes.size())
    {
        // Keep track of the current depth level and iterate through it
        mBoundingDepthOffsets.push_back(mBoundingIndexBuffer.GetSize());
        const size_t DepthLevel = TreeNodes.size();

        mBoundingVertexBuffer.Reserve(8 * (DepthLevel - NodeIdx));
        mBoundingIndexBuffer.Reserve(24 * (DepthLevel - NodeIdx));

        for (; NodeIdx < DepthLevel; NodeIdx++)
        {
            const SOBBTreeNode* pkNode = TreeNodes[NodeIdx];

            // Append children
            if (pkNode->NodeType == EOBBTreeNodeType::Branch)
            {
                const auto* pkBranch = static_cast<const SOBBTreeBranch*>(pkNode);
                TreeNodes.push_back(pkBranch->pLeft.get());
                TreeNodes.push_back(pkBranch->pRight.get());
            }

            // Create a new transform with the radii combined in as a scale matrie
            const auto CombinedTransform =
                pkNode->Transform * CTransform4f::ScaleMatrix(pkNode->Radii);

            // Transform a 1x1x1 unit cube using the transform...
            static constexpr std::array skUnitCubeVertices{
                CVector3f(-1, -1, -1),
                CVector3f(-1, -1,  1),
                CVector3f(-1,  1, -1),
                CVector3f(-1,  1,  1),
                CVector3f( 1, -1, -1),
                CVector3f( 1, -1,  1),
                CVector3f( 1,  1, -1),
                CVector3f( 1,  1,  1)
            };

            for (const auto& vert : skUnitCubeVertices)
            {
                const CVector3f Transformed = CombinedTransform * vert;
                mBoundingVertexBuffer.AddVertex(CVertex(Transformed));
            }

            // Add corresponding indices
            static constexpr std::array<uint16_t, 24> skUnitCubeWireIndices{
                0, 1,
                1, 3,
                3, 2,
                2, 0,
                4, 5,
                5, 7,
                7, 6,
                6, 4,
                0, 4,
                1, 5,
                2, 6,
                3, 7
            };

            const size_t FirstIndex = mBoundingVertexBuffer.Size() - 8;
            for (const auto index : skUnitCubeWireIndices)
            {
                mBoundingIndexBuffer.AddIndex(static_cast<uint16_t>(index + FirstIndex));
            }
        }
    }

    // Add an extra index at the end...
    mBoundingDepthOffsets.push_back(mBoundingIndexBuffer.GetSize());

    // Done
    mBoundingVertexBuffer.Buffer();
    mBoundingIndexBuffer.Buffer();
    mBoundingHierarchyBuilt = true;
}

/** Render */
void CCollisionRenderData::Render(bool Wireframe, int MaterialIndex /*= -1*/)
{
    mVertexBuffer.Bind();

    //@todo get these ugly OpenGL calls outta here
    if (Wireframe)
    {
        CDrawUtil::UseColorShader(CColor::Black());
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    if (MaterialIndex >= 0)
    {
        ASSERT( MaterialIndex < mMaterialIndexOffsets.size()-1 );
        const uint32_t FirstIndex = mMaterialIndexOffsets[MaterialIndex];
        const uint32_t NumIndices = mMaterialIndexOffsets[MaterialIndex+1] - FirstIndex;
        mIndexBuffer.DrawElements(FirstIndex, NumIndices);
    }
    else
    {
        mIndexBuffer.DrawElements();
    }

    if (Wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    mVertexBuffer.Unbind();
}

void CCollisionRenderData::RenderBoundingHierarchy(int MaxDepthLevel /*= -1*/)
{
    mBoundingVertexBuffer.Bind();
    CDrawUtil::UseColorShader(CColor::Blue());
    glLineWidth(1.f);
    const uint32_t FirstIndex = mBoundingDepthOffsets[0];
    const uint32_t LastIndex = (MaxDepthLevel > 0 ?
          mBoundingDepthOffsets[MaxDepthLevel] :
          mBoundingIndexBuffer.GetSize());
    const uint32_t NumIndices = LastIndex - FirstIndex;
    mBoundingIndexBuffer.DrawElements(FirstIndex, NumIndices);
    mBoundingVertexBuffer.Unbind();
}

int CCollisionRenderData::MaxBoundingHierarchyDepth() const
{
    return mBoundingHierarchyBuilt ? mBoundingDepthOffsets.size()-1 : 0;
}
