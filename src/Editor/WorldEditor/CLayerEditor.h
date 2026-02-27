#ifndef CLAYEREDITOR_H
#define CLAYEREDITOR_H

#include <Core/Resource/TResPtr.h>
#include <QDialog>

namespace Ui {
class CLayerEditor;
}

class CGameArea;
class CLayerModel;
class CScriptLayer;

class CLayerEditor : public QDialog
{
    Q_OBJECT
    TResPtr<CGameArea> mpArea;
    CLayerModel *mpModel;
    CScriptLayer *mpCurrentLayer = nullptr;

public:
    explicit CLayerEditor(QWidget *parent = nullptr);
    ~CLayerEditor() override;

    void SetArea(CGameArea *pArea);

private slots:
    void SetCurrentIndex(int Index);
    void EditLayerName(const QString& rkName);
    void EditLayerActive(bool Active);

private:
    std::unique_ptr<Ui::CLayerEditor> ui;
};

#endif // CLAYEREDITOR_H
