#ifndef CCHARACTEREDITOR_H
#define CCHARACTEREDITOR_H

#include "Editor/IEditor.h"
#include "Editor/CharacterEditor/CCharacterEditorViewport.h"
#include "Editor/CharacterEditor/CSkeletonHierarchyModel.h"

#include <memory>

namespace Ui {
class CCharacterEditor;
}

class CAnimation;
class CAnimSet;
class CBone;
class CCharacterNode;
class CScene;
class CSkeleton;
class QComboBox;

class CCharacterEditor : public IEditor
{
    Q_OBJECT

    std::unique_ptr<Ui::CCharacterEditor> ui;
    std::unique_ptr<CScene> mpScene;
    std::unique_ptr<CCharacterNode> mpCharNode;
    CBone *mpSelectedBone = nullptr;

    CSkeletonHierarchyModel mSkeletonModel;
    QComboBox *mpCharComboBox = nullptr;
    QComboBox *mpAnimComboBox = nullptr;

    TResPtr<CAnimSet> mpSet;
    uint32_t mCurrentChar = 0;
    uint32_t mCurrentAnim = 0;
    bool mBindPose = false;

    // Playback Controls
    bool mPlayAnim = true;
    bool mLoopAnim = true;
    float mAnimTime = 0.0f;
    float mPlaybackSpeed = 1.0f;

public:
    explicit CCharacterEditor(CAnimSet *pSet, QWidget *parent = nullptr);
    ~CCharacterEditor() override;

    void EditorTick(float DeltaTime) override;
    void UpdateAnimTime(float DeltaTime);
    void UpdateCameraOrbit();
    CSkeleton* CurrentSkeleton() const;
    CAnimation* CurrentAnimation() const;
    void SetActiveAnimSet(CAnimSet *pSet);
    void SetSelectedBone(CBone *pBone);
    CCharacterEditorViewport* Viewport() const override;

private slots:
    void ToggleGrid(bool Enable);
    void ToggleMeshVisible(bool Visible);
    void ToggleSkeletonVisible(bool Visible);
    void ToggleBindPose(bool Enable);
    void ToggleOrbit(bool Enable);
    void RefreshViewport();
    void OnViewportHoverBoneChanged(uint32_t BoneID);
    void OnViewportClick();
    void OnSkeletonTreeSelectionChanged(const QModelIndex& rkIndex);
    void SetActiveCharacterIndex(int CharIndex);
    void SetActiveAnimation(int AnimIndex);
    void PrevAnim();
    void NextAnim();

    void SetAnimTime(int Time);
    void SetAnimTime(float Time);
    void TogglePlay();
    void ToggleLoop(bool Loop);
    void Rewind();
    void FastForward();
    void AnimSpeedSpinBoxChanged(double NewVal);
};

#endif // CCHARACTEREDITORWINDOW_H
