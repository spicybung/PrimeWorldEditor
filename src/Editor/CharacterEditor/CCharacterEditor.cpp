#include "Editor/CharacterEditor/CCharacterEditor.h"
#include "ui_CCharacterEditor.h"

#include <Common/Macros.h>
#include "Editor/UICommon.h"
#include <Core/Scene/CScene.h>
#include <Core/Scene/CCharacterNode.h>

#include <QComboBox>
#include <QFileDialog>
#include <QTreeView>

#include <algorithm>

constexpr CVector3f skDefaultOrbitTarget{0, 0, 1};
constexpr float skDefaultOrbitDistance = 4.f;

CCharacterEditor::CCharacterEditor(CAnimSet *pSet, QWidget *parent)
    : IEditor(parent)
    , ui(std::make_unique<Ui::CCharacterEditor>())
    , mpScene(std::make_unique<CScene>())
{
    ui->setupUi(this);
    REPLACE_WINDOWTITLE_APPVARS;

    mpCharNode = std::make_unique<CCharacterNode>(mpScene.get(), UINT32_MAX);
    ui->Viewport->SetNode(mpCharNode.get());

    CCamera& rCamera = ui->Viewport->Camera();
    rCamera.SetMoveSpeed(0.5f);
    rCamera.SetPitch(-0.3f);
    rCamera.SetMoveMode(ECameraMoveMode::Orbit);

    // Init UI
    ui->ToolBar->addSeparator();

    mpCharComboBox = new QComboBox(this);
    mpCharComboBox->setMinimumWidth(175);
    ui->ToolBar->addWidget(mpCharComboBox);

    mpAnimComboBox = new QComboBox(this);
    mpAnimComboBox->setMinimumWidth(175);
    ui->ToolBar->addWidget(mpAnimComboBox);

    connect(ui->Viewport, &CCharacterEditorViewport::HoverBoneChanged, this, &CCharacterEditor::OnViewportHoverBoneChanged);
    connect(ui->Viewport, &CCharacterEditorViewport::ViewportClick, this, &CCharacterEditor::OnViewportClick);
    connect(ui->ActionShowGrid, &QAction::toggled, this, &CCharacterEditor::ToggleGrid);
    connect(ui->ActionShowMesh, &QAction::toggled, this, &CCharacterEditor::ToggleMeshVisible);
    connect(ui->ActionShowSkeleton, &QAction::toggled, this, &CCharacterEditor::ToggleSkeletonVisible);
    connect(ui->ActionBindPose, &QAction::toggled, this, &CCharacterEditor::ToggleBindPose);
    connect(ui->ActionOrbit, &QAction::toggled, this, &CCharacterEditor::ToggleOrbit);
    connect(ui->ActionPlay, &QAction::triggered, this, &CCharacterEditor::TogglePlay);
    connect(ui->ActionLoop, &QAction::toggled, this, &CCharacterEditor::ToggleLoop);
    connect(ui->ActionRewind, &QAction::triggered, this, &CCharacterEditor::Rewind);
    connect(ui->ActionFastForward, &QAction::triggered, this, &CCharacterEditor::FastForward);
    connect(ui->ActionPrevAnim, &QAction::triggered, this, &CCharacterEditor::PrevAnim);
    connect(ui->ActionNextAnim, &QAction::triggered, this, &CCharacterEditor::NextAnim);
    connect(mpCharComboBox, &QComboBox::currentIndexChanged, this, &CCharacterEditor::SetActiveCharacterIndex);
    connect(mpAnimComboBox, &QComboBox::currentIndexChanged, this, &CCharacterEditor::SetActiveAnimation);

    connect(ui->AnimSlider, &QSlider::valueChanged, this, qOverload<int>(&CCharacterEditor::SetAnimTime));
    connect(ui->PlayPauseButton, &QPushButton::pressed, this, &CCharacterEditor::TogglePlay);
    connect(ui->LoopButton, &QPushButton::toggled, this, &CCharacterEditor::ToggleLoop);
    connect(ui->RewindButton, &QPushButton::pressed, this, &CCharacterEditor::Rewind);
    connect(ui->FastForwardButton, &QPushButton::pressed, this, &CCharacterEditor::FastForward);
    connect(ui->AnimSpeedSpinBox, &WDraggableSpinBox::valueChanged, this, &CCharacterEditor::AnimSpeedSpinBoxChanged);

    // Init skeleton tree view
    ui->SkeletonHierarchyTreeView->setModel(&mSkeletonModel);
    const QList<int> SplitterSizes{static_cast<int>(width() * 0.2), static_cast<int>(width() * 0.8)};
    ui->splitter->setSizes(SplitterSizes);

    connect(ui->SkeletonHierarchyTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &CCharacterEditor::OnSkeletonTreeSelectionChanged);

    SetActiveAnimSet(pSet);
}

CCharacterEditor::~CCharacterEditor() = default;

void CCharacterEditor::EditorTick(float DeltaTime)
{
    UpdateAnimTime(DeltaTime);
    UpdateCameraOrbit();
}

void CCharacterEditor::UpdateAnimTime(float DeltaTime)
{
    const CAnimation *pAnim = CurrentAnimation();

    if (pAnim && mPlayAnim && !mBindPose && !ui->AnimSlider->isSliderDown())
    {
        mAnimTime += DeltaTime * mPlaybackSpeed;

        const float AnimLength = pAnim->Duration();

        if (mAnimTime > AnimLength)
        {
            if (mLoopAnim)
            {
                mAnimTime = fmodf(mAnimTime, AnimLength);
            }
            else
            {
                mAnimTime = AnimLength;
                TogglePlay();
            }
        }

        if (mAnimTime < 0.f)
        {
            if (mLoopAnim)
            {
                mAnimTime = AnimLength + fmodf(mAnimTime, AnimLength);
            }
            else
            {
                mAnimTime = 0.f;
                TogglePlay();
            }
        }

        SetAnimTime(mAnimTime);
    }
}

void CCharacterEditor::UpdateCameraOrbit()
{
    if (const CSkeleton* pSkel = CurrentSkeleton())
    {
        // If we have a selected bone, orbit around that.
        if (mpSelectedBone)
        {
            ui->Viewport->Camera().SetOrbitTarget(mpCharNode->BonePosition(mpSelectedBone->ID()));
        }
        else
        {
            // Otherwise, try to find Skeleton_Root. Barring that, we can orbit the root bone.
            const CBone* pRoot = pSkel->RootBone();
            const CBone* pSkelRoot = (pRoot ? pRoot->ChildByIndex(0) : pRoot);
            const CVector3f OrbitTarget = (pSkelRoot ? mpCharNode->BonePosition(pSkelRoot->ID()) : mpCharNode->CenterPoint());
            ui->Viewport->Camera().SetOrbitTarget(OrbitTarget);
        }
    }
    else
    {
        // Center around character if we have one, otherwise fall back to default orbit.
        if (mpSet)
            ui->Viewport->Camera().SetOrbitTarget(mpCharNode->CenterPoint());
        else
            ui->Viewport->Camera().SetOrbit(skDefaultOrbitTarget, skDefaultOrbitDistance);
    }
}

CSkeleton* CCharacterEditor::CurrentSkeleton() const
{
    return mpSet ? mpSet->Character(mCurrentChar)->pSkeleton : nullptr;
}

CAnimation* CCharacterEditor::CurrentAnimation() const
{
    return mpSet ? mpSet->FindAnimationAsset(mCurrentAnim) : nullptr;
}

void CCharacterEditor::SetActiveAnimSet(CAnimSet *pSet)
{
    mpSet = pSet;
    mpCharNode->SetCharSet(mpSet);
    SET_WINDOWTITLE_APPVARS(tr("%APP_FULL_NAME% - Character Editor: %1").arg(TO_QSTRING(mpSet->Source())));

    // Clear selected bone
    ui->SkeletonHierarchyTreeView->selectionModel()->clear();
    SetSelectedBone(nullptr);

    // Set up character combo box
    mpCharComboBox->blockSignals(true);
    mpCharComboBox->clear();

    for (const auto& character : mpSet->Characters())
        mpCharComboBox->addItem(TO_QSTRING(character.Name));

    SetActiveCharacterIndex(0);
    mpCharComboBox->blockSignals(false);

    // Set up anim combo box
    mpAnimComboBox->blockSignals(true);
    mpAnimComboBox->clear();

    for (const auto& animation : mpSet->Animations())
        mpAnimComboBox->addItem(TO_QSTRING(animation.Name));

    SetActiveAnimation(0);
    mpAnimComboBox->blockSignals(false);

    // Set up skeleton tree view
    CSkeleton *pSkel = mpSet->Character(mCurrentChar)->pSkeleton;
    mSkeletonModel.SetSkeleton(pSkel);
    ui->SkeletonHierarchyTreeView->expandAll();
    ui->SkeletonHierarchyTreeView->resizeColumnToContents(0);

    // Select first child bone of root (which should be Skeleton_Root) to line up the camera for orbiting.
    QModelIndex RootIndex = mSkeletonModel.index(0, 0);
    ui->SkeletonHierarchyTreeView->selectionModel()->setCurrentIndex( mSkeletonModel.index(0, 0, RootIndex), QItemSelectionModel::ClearAndSelect );

    // Run CCamera::SetOrbit to reset orbit distance.
    ui->Viewport->Camera().SetOrbit(mpCharNode->AABox());
}

void CCharacterEditor::SetSelectedBone(CBone *pBone)
{
    if (pBone != mpSelectedBone)
    {
        if (mpSelectedBone) mpSelectedBone->SetSelected(false);
        mpSelectedBone = pBone;
        if (mpSelectedBone) mpSelectedBone->SetSelected(true);
    }
}

CCharacterEditorViewport* CCharacterEditor::Viewport() const
{
    return ui->Viewport;
}

// ************ PUBLIC SLOTS ************
void CCharacterEditor::ToggleGrid(bool Enable)
{
    ui->Viewport->SetGridEnabled(Enable);
}

void CCharacterEditor::ToggleMeshVisible(bool Visible)
{
    // eShowObjectGeometry isn't the best fit, but close enough...?
    ui->Viewport->SetShowFlag(EShowFlag::ObjectGeometry, Visible);
}

void CCharacterEditor::ToggleSkeletonVisible(bool Visible)
{
    ui->Viewport->SetShowFlag(EShowFlag::Skeletons, Visible);
}

void CCharacterEditor::ToggleBindPose(bool Enable)
{
    mpCharNode->SetAnimated(!Enable);

    if (sender() != ui->ActionBindPose)
    {
        ui->ActionBindPose->blockSignals(true);
        ui->ActionBindPose->setChecked(Enable);
        ui->ActionBindPose->blockSignals(false);
    }

    if (Enable && mPlayAnim)
        TogglePlay();

    ui->AnimSlider->setEnabled(!Enable);
    mBindPose = Enable;
}

void CCharacterEditor::ToggleOrbit(bool Enable)
{
    ui->Viewport->Camera().SetMoveMode(Enable ? ECameraMoveMode::Orbit : ECameraMoveMode::Free);
}

void CCharacterEditor::RefreshViewport()
{
    ui->Viewport->ProcessInput();
    ui->Viewport->Render();
}

void CCharacterEditor::OnViewportHoverBoneChanged(uint32_t BoneID)
{
    if (BoneID == UINT32_MAX)
        ui->StatusBar->clearMessage();
    else
        ui->StatusBar->showMessage(tr("Bone %1: %2").arg(BoneID).arg(TO_QSTRING(mpSet->Character(mCurrentChar)->pSkeleton->BoneByID(BoneID)->Name())));
}

void CCharacterEditor::OnViewportClick()
{
    const uint32_t HoverBoneID = ui->Viewport->HoverBoneID();
    CSkeleton* pSkel = (mpSet ? mpSet->Character(mCurrentChar)->pSkeleton : nullptr);
    CBone* pBone = (pSkel ? pSkel->BoneByID(HoverBoneID) : nullptr);

    if (!pBone || !pBone->IsSelected())
    {
        if (pBone)
        {
            QModelIndex NewBoneIndex = mSkeletonModel.IndexForBone(pBone);
            ui->SkeletonHierarchyTreeView->selectionModel()->setCurrentIndex(NewBoneIndex, QItemSelectionModel::ClearAndSelect);
        }
        else
        {
            ui->SkeletonHierarchyTreeView->selectionModel()->clear();
        }

        SetSelectedBone(pBone);
    }
}

void CCharacterEditor::OnSkeletonTreeSelectionChanged(const QModelIndex& rkIndex)
{
    CBone *pBone = mSkeletonModel.BoneForIndex(rkIndex);
    SetSelectedBone(pBone);
}

void CCharacterEditor::SetActiveCharacterIndex(int CharIndex)
{
    mCurrentChar = CharIndex;
    mpCharNode->SetActiveChar(uint32_t(CharIndex));
}

void CCharacterEditor::SetActiveAnimation(int AnimIndex)
{
    mCurrentAnim = AnimIndex;
    mpCharNode->SetActiveAnim(uint32_t(AnimIndex));

    ui->AnimSlider->blockSignals(true);
    ui->AnimSlider->setMaximum((int) (CurrentAnimation() ? CurrentAnimation()->Duration() * 1000 : 0));
    ui->AnimSlider->blockSignals(false);

    mpAnimComboBox->blockSignals(true);
    mpAnimComboBox->setCurrentIndex(AnimIndex);
    mpAnimComboBox->blockSignals(false);

    SetAnimTime(0.f);
}

void CCharacterEditor::PrevAnim()
{
    if (mCurrentAnim > 0)
        SetActiveAnimation(mCurrentAnim - 1);
}

void CCharacterEditor::NextAnim()
{
    const size_t MaxAnim = (mpSet ? mpSet->NumAnimations() - 1 : 0);
    if (mCurrentAnim < MaxAnim)
        SetActiveAnimation(mCurrentAnim + 1);
}

void CCharacterEditor::SetAnimTime(int Time)
{
    float FloatTime = Time / 1000.f;
    SetAnimTime(FloatTime);
}

void CCharacterEditor::SetAnimTime(float Time)
{
    if (mBindPose)
        Time = 0.f;

    mAnimTime = Time;

    if (ui->AnimSlider != sender() || mBindPose)
    {
        const auto IntTime = static_cast<int>(Time * 1000);
        ui->AnimSlider->setValue(IntTime);
    }

    mpCharNode->SetAnimTime(Time);

    const CAnimation* pAnim = CurrentAnimation();
    uint32_t NumKeys = 1;
    uint32_t CurKey = 0;

    if (pAnim)
    {
        NumKeys = pAnim->NumKeys();
        CurKey = std::min(static_cast<uint32_t>(Time / pAnim->TickInterval()) + 1, NumKeys - 1);
    }

    ui->FrameLabel->setText(tr("Frame %1 / %2 (%3s/%4s)").arg(CurKey).arg(NumKeys - 1).arg(mAnimTime, 0, 'f', 3).arg(pAnim ? pAnim->Duration() : 0.f, 0, 'f', 3));
}

void CCharacterEditor::TogglePlay()
{
    if (mBindPose)
        ToggleBindPose(false);

    mPlayAnim = !mPlayAnim;
    const QString NewText = (mPlayAnim ? tr("Pause") : tr("Play"));
    ui->PlayPauseButton->setToolTip(NewText);
    ui->ActionPlay->setText(NewText);

    const QIcon PlayPauseIcon = QIcon(mPlayAnim ? QStringLiteral(":/icons/Pause_24px.svg") : QStringLiteral(":/icons/Play_24px.svg"));
    ui->PlayPauseButton->setIcon(PlayPauseIcon);

    if (ui->ActionPlay != sender())
    {
        ui->ActionPlay->blockSignals(true);
        ui->ActionPlay->setChecked(mPlayAnim);
        ui->ActionPlay->blockSignals(false);
    }

    CAnimation *pAnim = CurrentAnimation();

    if (pAnim && mPlayAnim)
    {
        if (mPlaybackSpeed < 0.f && mAnimTime == 0.f)
            SetAnimTime(pAnim->Duration());
        if (mPlaybackSpeed >= 0.f && mAnimTime == pAnim->Duration())
            SetAnimTime(0.f);
    }
}

void CCharacterEditor::ToggleLoop(bool Loop)
{
    mLoopAnim = Loop;

    const QString NewText = (Loop ? tr("Disable Loop") : tr("Loop"));
    ui->LoopButton->setToolTip(NewText);
    ui->ActionLoop->setText(NewText);

    const QIcon ActionIcon = QIcon(Loop ? QStringLiteral(":/icons/DontLoop_24px") : QStringLiteral(":/icons/Loop_24px.svg"));
    ui->ActionLoop->setIcon(ActionIcon);

    if (sender() != ui->LoopButton)
    {
        ui->LoopButton->blockSignals(true);
        ui->LoopButton->setChecked(Loop);
        ui->LoopButton->blockSignals(false);
    }

    if (sender() != ui->ActionLoop)
    {
        ui->LoopButton->blockSignals(true);
        ui->ActionLoop->setChecked(Loop);
        ui->LoopButton->blockSignals(false);
    }
}

void CCharacterEditor::Rewind()
{
    SetAnimTime(0.f);
}

void CCharacterEditor::FastForward()
{
    CAnimation *pAnim = CurrentAnimation();
    if (pAnim && !mBindPose) SetAnimTime(pAnim->Duration());
}

void CCharacterEditor::AnimSpeedSpinBoxChanged(double NewVal)
{
    mPlaybackSpeed = (float) NewVal;
}
