#ifndef CTEMPLATEEDITDIALOG_H
#define CTEMPLATEEDITDIALOG_H

#include <Common/EGame.h>
#include <Common/TString.h>
#include <Core/Resource/Script/Property/IProperty.h>

#include <QDialog>
#include <memory>

namespace Ui {
class CTemplateEditDialog;
}

class CPropertyNameValidator;
class IProperty;

class CTemplateEditDialog : public QDialog
{
    Q_OBJECT
    std::unique_ptr<Ui::CTemplateEditDialog> mpUI;
    CPropertyNameValidator* mpValidator;

    IProperty *mpProperty;
    EGame mGame;

    TString mOriginalName;
    TString mOriginalDescription;
    TString mOriginalTypeName;
    bool mOriginalAllowTypeNameOverride = false;
    bool mOriginalNameWasValid = true;

    // These members help track what templates need to be updated and resaved after the user clicks OK
    QList<IProperty*> mEquivalentProperties;

public:
    explicit CTemplateEditDialog(IProperty* pProperty, QWidget *pParent = nullptr);
    ~CTemplateEditDialog() override;

signals:
    void PerformedTypeConversion();

protected:
    void UpdateDescription(const TString& rkNewDesc);
    void UpdateTypeName(const TString& kNewTypeName, bool AllowOverride);
    void FindEquivalentProperties(IProperty *pProperty);

private slots:
    void ApplyChanges();
    void RefreshTypeNameOverride();

    void ConvertPropertyType(EPropertyType Type);
    void ConvertToInt();
    void ConvertToChoice();
    void ConvertToSound();
    void ConvertToFlags();
};

#endif // CTEMPLATEEDITDIALOG_H
