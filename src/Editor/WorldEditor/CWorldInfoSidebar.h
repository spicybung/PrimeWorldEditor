#ifndef CWORLDINFOSIDEBAR_H
#define CWORLDINFOSIDEBAR_H

#include "Editor/WorldEditor/CWorldEditorSidebar.h"
#include "Editor/WorldEditor/CWorldTreeModel.h"

#include <memory>

class CGameProject;
class CWorldEditor;

namespace Ui {
class CWorldInfoSidebar;
}

class CWorldInfoSidebar : public CWorldEditorSidebar
{
    Q_OBJECT

    std::unique_ptr<Ui::CWorldInfoSidebar> mpUI;
    CWorldTreeModel mModel;
    CWorldTreeProxyModel mProxyModel;

public:
    explicit CWorldInfoSidebar(CWorldEditor *pEditor);
    ~CWorldInfoSidebar() override;

private slots:
    void OnActiveProjectChanged(const CGameProject* pProj);
    void OnAreaFilterStringChanged(const QString& rkFilter);
    void OnWorldTreeClicked(const QModelIndex& Index);
    void OnWorldTreeDoubleClicked(const QModelIndex& Index);
    void ClearWorldInfo();
    void ClearAreaInfo();
};

#endif // CWORLDINFOSIDEBAR_H
