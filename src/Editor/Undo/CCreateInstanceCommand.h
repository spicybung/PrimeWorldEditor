#ifndef CCREATEINSTANCECOMMAND_H
#define CCREATEINSTANCECOMMAND_H

#include "Editor/Undo/IUndoCommand.h"
#include "Editor/Undo/ObjReferences.h"

class CGameArea;
class CScene;
class CScriptLayer;
class CScriptTemplate;
class CWorldEditor;

class CCreateInstanceCommand : public IUndoCommand
{
    CWorldEditor *mpEditor;
    CScene *mpScene;
    CGameArea *mpArea;
    CScriptTemplate *mpTemplate;
    uint32_t mLayerIndex;
    CVector3f mSpawnPosition;

    CNodePtrList mOldSelection;
    CInstancePtr mpNewInstance;
    CNodePtr mpNewNode;

public:
    CCreateInstanceCommand(CWorldEditor *pEditor, CScriptTemplate *pTemplate, CScriptLayer *pLayer, const CVector3f& rkPosition);
    void undo() override;
    void redo() override;
    bool AffectsCleanState() const override { return true; }
};

#endif // CCREATEINSTANCECOMMAND_H
