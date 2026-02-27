#ifndef CSTRINGEDITOR_H
#define CSTRINGEDITOR_H

#include "Editor/IEditor.h"
#include <Core/Resource/TResPtr.h>
#include <Core/Resource/StringTable/ELanguage.h>

#include <memory>

namespace Ui {
class CStringEditor;
}

class CStringListModel;
class CStringTable;

/** Editor window for string tables (STRG assets) */
class CStringEditor : public IEditor
{
    Q_OBJECT

    /** Qt UI */
    std::unique_ptr<Ui::CStringEditor> mpUI;

    /** String table asset being edited */
    TResPtr<CStringTable> mpStringTable;

    /** Language being edited */
    ELanguage mCurrentLanguage{ELanguage::English};

    /** Index of the string being edited */
    uint32_t mCurrentStringIndex = UINT32_MAX;

    /** Current string count */
    uint32_t mCurrentStringCount = 0;

    /** Model for the string list view */
    CStringListModel* mpListModel = nullptr;

    /** Editor state flags */
    bool mIsEditingStringName = false;
    bool mIsEditingStringData = false;

public:
    explicit CStringEditor(CStringTable* pStringTable, QWidget* pParent = nullptr);
    ~CStringEditor() override;

    bool Save() override;
    bool eventFilter(QObject* pWatched, QEvent* pEvent) override;

    void InitUI();
    void UpdateStatusBar();
    void SetActiveLanguage(ELanguage Language);
    void SetActiveString(int StringIndex);

    void LoadSettings();
    void SaveSettings();

    // Accessors
    CStringTable* StringTable() const { return mpStringTable; }

signals:
    void StringNameEdited();
    void StringEdited();

public slots:
    void OnMoveString(int StringIndex, int NewIndex);

private slots:
    void UpdateUI();
    void OnStringSelected(const QModelIndex& kIndex);
    void OnLanguageChanged(int LanguageIndex);
    void OnStringNameEdited();
    void OnStringTextEdited();
    void OnAddString();
    void OnRemoveString();
    
    void IncrementStringIndex();
    void DecrementStringIndex();
    void IncrementLanguageIndex();
    void DecrementLanguageIndex();
};

#endif // CSTRINGEDITOR_H
