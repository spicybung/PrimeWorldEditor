#ifndef CCOLLISIONRENDERSETTINGSDIALOG_H
#define CCOLLISIONRENDERSETTINGSDIALOG_H

#include <QDialog>
#include <memory>

class CWorldEditor;

namespace Ui {
class CCollisionRenderSettingsDialog;
}

class CCollisionRenderSettingsDialog : public QDialog
{
    Q_OBJECT
    std::unique_ptr<Ui::CCollisionRenderSettingsDialog> mpUi;

    CWorldEditor *mpEditor;

public:
    explicit CCollisionRenderSettingsDialog(CWorldEditor *pEditor, QWidget *pParent = nullptr);
    ~CCollisionRenderSettingsDialog() override;

private slots:
    void SetupWidgets();
    void OnHideMaskChanged(const QString& NewMask);
    void OnHighlightMaskChanged(const QString& NewMask);
    void OnWireframeToggled(bool Enable);
    void OnSurfaceTypeToggled(bool Enable);
    void OnStandableTrisToggled(bool Enable);
    void OnAreaBoundsToggled(bool Enable);
    void OnBackfacesToggled(bool Enable);
    void OnHideCheckboxesToggled();
};

#endif // CCOLLISIONRENDERSETTINGSDIALOG_H
