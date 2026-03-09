#ifndef CVERTEXARRAYMANAGER_H
#define CVERTEXARRAYMANAGER_H

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <GL/glew.h>

class CDynamicVertexBuffer;
class CVertexBuffer;

class CVertexArrayManager
{
    std::unordered_map<CVertexBuffer*, GLuint> mVBOMap;
    std::unordered_map<CDynamicVertexBuffer*, GLuint> mDynamicVBOMap;
    uint32_t mVectorIndex = 0;

    static std::vector<CVertexArrayManager*> sVAManagers;
    static CVertexArrayManager *spCurrentManager;

public:
    CVertexArrayManager();
    ~CVertexArrayManager();

    void SetCurrent();
    void BindVAO(CVertexBuffer *pVBO);
    void BindVAO(CDynamicVertexBuffer *pVBO);
    void DeleteVAO(CVertexBuffer *pVBO);
    void DeleteVAO(CDynamicVertexBuffer *pVBO);

    static CVertexArrayManager* Current();
    static void DeleteAllArraysForVBO(CVertexBuffer *pVBO);
    static void DeleteAllArraysForVBO(CDynamicVertexBuffer *pVBO);
};

#endif // CVERTEXARRAYMANAGER_H
