#ifndef CQUICKPLAYPROPERTYEDITOR_H
#define CQUICKPLAYPROPERTYEDITOR_H

#include <QMenu>
#include <memory>

namespace Ui {
class CQuickplayPropertyEditor;
}

class CGameArea;
class CWorld;
class QListWidgetItem;

struct SQuickplayParameters;

/** Property editor widget for quickplay.
 *  @todo may want this to use a CPropertyView eventually.
 */
class CQuickplayPropertyEditor : public QMenu
{
    Q_OBJECT

    std::unique_ptr<Ui::CQuickplayPropertyEditor> mpUI;
    SQuickplayParameters& mParameters;

public:
    explicit CQuickplayPropertyEditor(SQuickplayParameters& Parameters, QWidget* pParent = nullptr);
    ~CQuickplayPropertyEditor() override;

private slots:
    void BrowseForDolphin();
    void OnDolphinPathChanged(const QString& kNewPath);
    void OnBootToAreaToggled(bool Enabled);
    void OnSpawnAtCameraLocationToggled(bool Enabled);
    void OnGiveAllItemsToggled(bool Enabled);
    void OnLayerListItemChanged(const QListWidgetItem* pItem);

    void OnWorldEditorAreaChanged(const CWorld* pWorld, const CGameArea* pArea);
};

#endif // CQUICKPLAYPROPERTYEDITOR_H
