#ifndef CSELECTRESOURCEPANEL_H
#define CSELECTRESOURCEPANEL_H

#include "Editor/Widgets/CFilteredResourceModel.h"

#include <QFrame>
#include <memory>

namespace Ui {
class CSelectResourcePanel;
}

class CResourceSelector;

class CSelectResourcePanel : public QFrame
{
    Q_OBJECT
    std::unique_ptr<Ui::CSelectResourcePanel> mpUI;
    CResourceSelector *mpSelector;

    CFilteredResourceModel mModel;
    CFilteredResourceProxyModel mProxyModel;

public:
    explicit CSelectResourcePanel(CResourceSelector *pSelector);
    ~CSelectResourcePanel() override;

private slots:
    void FocusChanged(QWidget *pOld, QWidget *pNew);
    void SearchStringChanged(const QString& SearchString);
    void ResourceClicked(const QModelIndex& Index);
};

#endif // CSELECTRESOURCEPANEL_H
