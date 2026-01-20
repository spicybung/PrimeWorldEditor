#include "Editor/WorldEditor/WCreateTab.h"
#include "ui_WCreateTab.h"

#include "Editor/Undo/UndoCommands.h"
#include "Editor/WorldEditor/CTemplateMimeData.h"
#include "Editor/WorldEditor/CWorldEditor.h"
#include <Core/GameProject/CGameProject.h>
#include <Core/Resource/Script/CScriptLayer.h>
#include <Core/Resource/Script/NGameList.h>

WCreateTab::WCreateTab(CWorldEditor *pEditor, QWidget *pParent)
    : QWidget(pParent)
    , ui(std::make_unique<Ui::WCreateTab>())
{
    ui->setupUi(this);

    mpEditor = pEditor;
    mpEditor->Viewport()->installEventFilter(this);

    connect(mpEditor, &CWorldEditor::LayersModified, this, &WCreateTab::OnLayersChanged);
    connect(gpEdApp, &CEditorApplication::ActiveProjectChanged, this, &WCreateTab::OnActiveProjectChanged);
    connect(ui->SpawnLayerComboBox, &QComboBox::currentIndexChanged, this, &WCreateTab::OnSpawnLayerChanged);
}

WCreateTab::~WCreateTab() = default;

bool WCreateTab::eventFilter(QObject *pObj, QEvent *pEvent)
{
    if (pObj == mpEditor->Viewport())
    {
        if (pEvent->type() == QEvent::DragEnter)
        {
            if (mpEditor->ActiveArea() != nullptr)
            {
                QDragEnterEvent *pDragEvent = static_cast<QDragEnterEvent*>(pEvent);

                if (qobject_cast<const CTemplateMimeData*>(pDragEvent->mimeData()))
                {
                    pDragEvent->acceptProposedAction();
                    return true;
                }
            }
        }

        else if (pEvent->type() == QEvent::Drop)
        {
            const auto* pDropEvent = static_cast<QDropEvent*>(pEvent);
            const auto* pMimeData = qobject_cast<const CTemplateMimeData*>(pDropEvent->mimeData());

            if (pMimeData)
            {
                const CVector3f& SpawnPoint = mpEditor->Viewport()->HoverPoint();
                auto* pCmd = new CCreateInstanceCommand(mpEditor, pMimeData->Template(), mpSpawnLayer, SpawnPoint);
                mpEditor->UndoStack().push(pCmd);
                return true;
            }
        }
    }

    return false;
}

// ************ PUBLIC SLOTS ************
void WCreateTab::OnActiveProjectChanged(CGameProject *pProj)
{
    EGame Game = (pProj ? pProj->Game() : EGame::Invalid);
    CGameTemplate *pGame = NGameList::GetGameTemplate(Game);
    ui->TemplateView->SetGame(pGame);
}

void WCreateTab::OnLayersChanged()
{
    CGameArea *pArea = mpEditor->ActiveArea();

    ui->SpawnLayerComboBox->blockSignals(true);
    ui->SpawnLayerComboBox->clear();

    for (const auto* layer : pArea->ScriptLayers())
        ui->SpawnLayerComboBox->addItem(TO_QSTRING(layer->Name()));

    ui->SpawnLayerComboBox->setCurrentIndex(0);
    ui->SpawnLayerComboBox->blockSignals(false);

    OnSpawnLayerChanged(0);
}

void WCreateTab::OnSpawnLayerChanged(int LayerIndex)
{
    CGameArea *pArea = mpEditor->ActiveArea();
    mpSpawnLayer = pArea->ScriptLayer(static_cast<size_t>(LayerIndex));
}
