#include "Editor/Widgets/CSelectResourcePanel.h"
#include "ui_CSelectResourcePanel.h"

#include "Editor/CEditorApplication.h"
#include "Editor/Widgets/CResourceSelector.h"

CSelectResourcePanel::CSelectResourcePanel(CResourceSelector *pSelector)
    : QFrame(pSelector)
    , mpUI(std::make_unique<Ui::CSelectResourcePanel>())
    , mpSelector(pSelector)
    , mModel(pSelector)
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::Window);

    mpUI->setupUi(this);
    mProxyModel.setSourceModel(&mModel);
    mpUI->ResourceTableView->setModel(&mProxyModel);

    // Signals/slots
    connect(gpEdApp, &QApplication::focusChanged, this, &CSelectResourcePanel::FocusChanged);
    connect(mpUI->SearchBar, &CTimedLineEdit::StoppedTyping, this, &CSelectResourcePanel::SearchStringChanged);
    connect(mpUI->ResourceTableView, &QTableView::clicked, this, &CSelectResourcePanel::ResourceClicked);

    // Determine selector location
    const QPoint SelectorPos = pSelector->parentWidget()->mapToGlobal(pSelector->pos());
    move(SelectorPos);

    // Jump to the currently-selected resource
    const QModelIndex Index = mModel.InitialIndex();
    const QModelIndex ProxyIndex = mProxyModel.mapFromSource(Index);

    mpUI->ResourceTableView->scrollTo(ProxyIndex, QAbstractItemView::PositionAtCenter);
    mpUI->ResourceTableView->selectionModel()->setCurrentIndex(ProxyIndex, QItemSelectionModel::ClearAndSelect);

    // Show
    show();
    mpUI->SearchBar->setFocus();
}

CSelectResourcePanel::~CSelectResourcePanel() = default;

// Slots
void CSelectResourcePanel::FocusChanged(QWidget*, QWidget *pNew)
{
    // Destroy when the panel loses focus
    if (pNew != this && !isAncestorOf(pNew))
        deleteLater();
}

void CSelectResourcePanel::SearchStringChanged(const QString& SearchString)
{
    mProxyModel.SetSearchString(SearchString);
}

void CSelectResourcePanel::ResourceClicked(const QModelIndex& Index)
{
    const QModelIndex SourceIndex = mProxyModel.mapToSource(Index);
    CResourceEntry* pEntry = mModel.EntryForIndex(SourceIndex);
    mpSelector->SetResourceEntry(pEntry);
    close();
}
