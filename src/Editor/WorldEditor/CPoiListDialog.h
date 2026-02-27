#ifndef CPOILISTDIALOG_H
#define CPOILISTDIALOG_H

#include <QAbstractListModel>
#include <QDialog>
#include <QList>
#include <QSortFilterProxyModel>

class CPoiMapModel;
class CScene;
class CScriptNode;
class CScriptTemplate;
class QDialogButtonBox;
class QListView;

class CPoiListModel : public QAbstractListModel
{
    Q_OBJECT

    CScriptTemplate* mpPoiTemplate;
    QList<CScriptNode*> mObjList;

public:
    explicit CPoiListModel(CScriptTemplate* pPoiTemplate, const CPoiMapModel* pMapModel, CScene* pScene, QWidget* pParent = nullptr);
    ~CPoiListModel() override;

    int rowCount(const QModelIndex&) const override;
    QVariant data(const QModelIndex& rkIndex, int Role) const override;

    CScriptNode* PoiForIndex(const QModelIndex& rkIndex) const;
};

class CPoiListDialog : public QDialog
{
    Q_OBJECT

    CPoiListModel mSourceModel;
    QSortFilterProxyModel mModel;
    QList<CScriptNode*> mSelection;

    QListView* mpListView;
    QDialogButtonBox* mpButtonBox;

public:
    explicit CPoiListDialog(CScriptTemplate* pPoiTemplate, const CPoiMapModel* pMapModel, CScene* pScene, QWidget* pParent = nullptr);
    ~CPoiListDialog() override;

    const QList<CScriptNode*>& Selection() const { return mSelection; }

private slots:
    void OnOkClicked();
    void OnCancelClicked();
};

#endif // CPOILISTDIALOG_H
