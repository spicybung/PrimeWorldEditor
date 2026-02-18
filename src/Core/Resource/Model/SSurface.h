#ifndef SSURFACE_H
#define SSURFACE_H

#include "Core/OpenGL/GLCommon.h"
#include "Core/Resource/Model/CVertex.h"
#include <Common/Math/CAABox.h>
#include <Common/Math/CVector3f.h>
#include <cstdint>
#include <vector>

// Should prolly be a class
struct SSurface
{
    uint32_t VertexCount = 0;
    uint32_t TriangleCount = 0;
    CAABox AABox;
    CVector3f CenterPoint;
    uint32_t MaterialID = 0;
    CVector3f ReflectionDirection;
    uint16_t MeshID = 0;

    struct SPrimitive
    {
        EPrimitiveType Type{};
        std::vector<CVertex> Vertices;
    };
    std::vector<SPrimitive> Primitives;

    SSurface() = default;

    std::pair<bool,float> IntersectsRay(const CRay& rkRay, bool AllowBackfaces = false, float LineThreshold = 0.02f) const;
};

#endif // SSURFACE_H
