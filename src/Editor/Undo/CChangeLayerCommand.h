#ifndef CCHANGELAYERCOMMAND_H
#define CCHANGELAYERCOMMAND_H

#include "Editor/Undo/IUndoCommand.h"
#include "Editor/Undo/ObjReferences.h"

#include <QMap>
#include <span>

class CScriptLayer;
class CScriptNode;
class CWorldEditor;

class CChangeLayerCommand : public IUndoCommand
{
    CNodePtrList mNodes;
    QMap<uint32_t, CScriptLayer*> mOldLayers;
    CScriptLayer *mpNewLayer;
    CWorldEditor *mpEditor;

public:
    CChangeLayerCommand(CWorldEditor *pEditor, std::span<CScriptNode*> nodeList, CScriptLayer *pNewLayer);
    void undo() override;
    void redo() override;
    bool AffectsCleanState() const override { return true; }
};

#endif // CCHANGELAYERCOMMAND_H
