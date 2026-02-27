#ifndef CPROJECTSETTINGSDIALOG_H
#define CPROJECTSETTINGSDIALOG_H

#include <Core/Resource/TResPtr.h>
#include <QDialog>
#include <QList>

#include <memory>

namespace Ui {
class CProjectSettingsDialog;
}

class CGameProject;
class CResourceEntry;
class CWorld;

class CProjectSettingsDialog : public QDialog
{
    Q_OBJECT
    std::unique_ptr<Ui::CProjectSettingsDialog> mpUI;
    CGameProject *mpProject = nullptr;

    QList<CResourceEntry*> mWorldEntries;
    QList<CResourceEntry*> mAreaEntries;
    TResPtr<CWorld> mpWorld;

public:
    explicit CProjectSettingsDialog(QWidget *pParent = nullptr);
    ~CProjectSettingsDialog() override;

private slots:
    void ActiveProjectChanged(CGameProject *pProj);
    void GameNameChanged();
    void SetupPackagesList();
    void CookPackage();
    void CookAllDirtyPackages();
    void BuildISO();
};

#endif // CPROJECTSETTINGSDIALOG_H
