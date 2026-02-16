#ifndef SRAYINTERSECTION
#define SRAYINTERSECTION

#include <Common/Math/CVector3f.h>
#include <cstdint>

class CSceneNode;

struct SRayIntersection
{
    bool Hit = false;
    float Distance = 0.0f;
    CVector3f HitPoint{CVector3f::Zero()};
    CSceneNode *pNode = nullptr;
    uint32_t ComponentIndex = UINT32_MAX;
};

#endif // SRAYINTERSECTION

