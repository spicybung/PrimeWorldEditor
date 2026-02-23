#ifndef INODEEDITOR_H
#define INODEEDITOR_H

#include "Editor/CGizmo.h"
#include "Editor/IEditor.h"
#include "Editor/CNodeSelection.h"
#include <Common/Math/ETransformSpace.h>
#include <Core/Scene/CScene.h>

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QList>

#include <memory>

class CScriptObject;

class INodeEditor : public IEditor
{
    Q_OBJECT
public:
    enum class ECloneState
    {
        NotCloning,
        ReadyToClone,
        Cloning
    };
protected:
    // Node management
    CScene mScene;
    std::unique_ptr<CNodeSelection> mpSelection;
    bool mSelectionLocked = false;

    // Gizmo
    CGizmo mGizmo;
    bool mShowGizmo = false;
    bool mGizmoHovering = false;
    bool mGizmoTransforming = false;
    ETransformSpace mTranslateSpace{ETransformSpace::World};
    ETransformSpace mRotateSpace{ETransformSpace::World};
    ECloneState mCloneState{ECloneState::NotCloning};

    // Gizmo widgets
    QActionGroup *mpGizmoGroup = nullptr;
    QList<QAction*> mGizmoActions;
    QComboBox *mpTransformCombo = nullptr;

    // Pick mode
    bool mPickMode = false;
    bool mExitOnInvalidPick = false;
    bool mEmitOnInvalidPick = false;
    bool mEmitOnButtonPress = false;
    FNodeFlags mAllowedPickNodes{};
    CSceneNode *mpPickHoverNode = nullptr;
    Qt::MouseButtons mPickButtons{};
    Qt::KeyboardModifiers mPickModifiers{};

public:
    explicit INodeEditor(QWidget *pParent = nullptr);
    ~INodeEditor() override;

    CScene* Scene();
    CGizmo* Gizmo();
    bool IsGizmoVisible() const;
    void BeginGizmoTransform();
    void EndGizmoTransform();
    ETransformSpace CurrentTransformSpace() const;

    void SelectNode(CSceneNode *pNode);
    void BatchSelectNodes(QList<CSceneNode*> Nodes, bool ClearExistingSelection, const QString& rkCommandName = QStringLiteral("Select"));
    void DeselectNode(CSceneNode *pNode);
    void BatchDeselectNodes(QList<CSceneNode*> Nodes, const QString& rkCommandName = QStringLiteral("Deselect"));
    void ClearSelection();
    void ClearAndSelectNode(CSceneNode *pNode);
    void SelectAll(FNodeFlags NodeFlags);
    void InvertSelection(FNodeFlags NodeFlags);
    void SetSelectionLocked(bool Locked);
    bool HasSelection() const;
    CNodeSelection* Selection() const;

    void EnterPickMode(FNodeFlags AllowedNodes, bool ExitOnInvalidPick, bool EmitOnInvalidPick, bool EmitHoverOnButtonPress, const QCursor& Cursor = Qt::CrossCursor);
    void ExitPickMode();

    void NotifySelectionTransformed();
    void NotifyNodeAboutToBeSpawned();
    void NotifyNodeSpawned(CSceneNode *pNode);
    void NotifyNodeAboutToBeDeleted(CSceneNode *pNode);
    void NotifyNodeDeleted();

signals:
    void InstanceLinksModified(const QList<CScriptObject*>& rkInstances);

    void NodeAboutToBeSpawned();
    void NodeSpawned(CSceneNode *pNode);
    void NodeAboutToBeDeleted(CSceneNode *pNode);
    void NodeDeleted();
    void SelectionModified();
    void SelectionTransformed();

    void PickModeEntered(const QCursor& Cursor);
    void PickModeExited();
    void PickModeClick(const SRayIntersection& rkRayIntersect, QMouseEvent *pEvent);
    void PickModeHoverChanged(const SRayIntersection& rkRayIntersect, QMouseEvent *pEvent);

public slots:
    void OnLinksModified(const QList<CScriptObject*>& rkInstances);
    void OnSelectionModified();
    void OnGizmoMoved();
    virtual void UpdateGizmoUI() = 0;
    virtual void UpdateSelectionUI() = 0;

protected:
    virtual void GizmoModeChanged(CGizmo::EGizmoMode /*mode*/) {}

protected slots:
    void OnViewportClick(const SRayIntersection& rkRayIntersect, QMouseEvent *pEvent);
    void OnViewportInputProcessed(const SRayIntersection& rkRayIntersect, QMouseEvent *pEvent);

private:
    void UpdateTransformActionsEnabled();

private slots:
    void OnSelectObjectsTriggered();
    void OnTranslateTriggered();
    void OnRotateTriggered();
    void OnScaleTriggered();
    void OnTransformSpaceChanged(int SpaceIndex);
};

#endif // INODEEDITOR_H
