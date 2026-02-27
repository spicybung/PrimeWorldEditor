#ifndef CSELECTINSTANCEDIALOG_H
#define CSELECTINSTANCEDIALOG_H

#include "Editor/WorldEditor/CInstancesModel.h"
#include "Editor/WorldEditor/CInstancesProxyModel.h"
#include <QDialog>

#include <memory>

namespace Ui {
class CSelectInstanceDialog;
}

class CSelectInstanceDialog : public QDialog
{
    Q_OBJECT

    CWorldEditor *mpEditor;
    CInstancesModel mLayersModel;
    CInstancesModel mTypesModel;
    CInstancesProxyModel mLayersProxyModel;
    CInstancesProxyModel mTypesProxyModel;

    bool mValidSelection = false;
    CScriptObject *mpLayersInst = nullptr;
    CScriptObject *mpTypesInst = nullptr;

    std::unique_ptr<Ui::CSelectInstanceDialog> ui;

public:
    explicit CSelectInstanceDialog(CWorldEditor *pEditor, QWidget *pParent = nullptr);
    ~CSelectInstanceDialog() override;

    CScriptObject* SelectedInstance() const;

private slots:
    void OnTabChanged(int NewTabIndex);
    void OnTreeClicked(const QModelIndex& Index);
    void OnTreeDoubleClicked(const QModelIndex& Index);
};

#endif // CSELECTINSTANCEDIALOG_H
