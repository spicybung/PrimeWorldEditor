#ifndef CMODELEDITORWINDOW_H
#define CMODELEDITORWINDOW_H

#include "Editor/IEditor.h"
#include "Editor/ModelEditor/CModelEditorViewport.h"
#include <Core/Resource/ETevEnums.h>

#include <memory>

namespace Ui {
class CModelEditorWindow;
}

class CMaterial;
class CMaterialPass;
class CModel;
class CModelNode;
class CResourceEntry;
class CScene;

// the model editor is messy and old as fuck, it needs a total rewrite
class CModelEditorWindow : public IEditor
{
    Q_OBJECT

    std::unique_ptr<Ui::CModelEditorWindow> ui;
    std::unique_ptr<CScene> mpScene;
    QString mOutputFilename;
    TResPtr<CModel> mpCurrentModel;
    std::unique_ptr<CModelNode> mpCurrentModelNode;
    CMaterial *mpCurrentMat = nullptr;
    CMaterialPass *mpCurrentPass = nullptr;
    bool mIgnoreSignals = false;

public:
    explicit CModelEditorWindow(CModel *pModel, QWidget *pParent = nullptr);
    ~CModelEditorWindow() override;

    bool Save() override;
    void SetActiveModel(CModel *pModel);
    CModelEditorViewport* Viewport() const override;

private:
    enum class EModelEditorWidget
    {
        SetSelectComboBox,
        MatSelectComboBox,
        EnableTransparencyCheckBox,
        EnablePunchthroughCheckBox,
        EnableReflectionCheckBox,
        EnableSurfaceReflectionCheckBox,
        EnableDepthWriteCheckBox,
        EnableOccluderCheckBox,
        EnableLightmapCheckBox,
        EnableLightingCheckBox,
        SourceBlendComboBox,
        DestBlendComboBox,
        IndTextureResSelector,
        KonstColorPickerA,
        KonstColorPickerB,
        KonstColorPickerC,
        KonstColorPickerD,
        PassTableWidget,
        TevKColorSelComboBox,
        TevKAlphaSelComboBox,
        TevRasSelComboBox,
        TevTexSelComboBox,
        TevTexSourceComboBox,
        PassTextureResSelector,
        TevColorComboBoxA,
        TevColorComboBoxB,
        TevColorComboBoxC,
        TevColorComboBoxD,
        TevColorOutputComboBox,
        TevAlphaComboBoxA,
        TevAlphaComboBoxB,
        TevAlphaComboBoxC,
        TevAlphaComboBoxD,
        TevAlphaOutputComboBox,
        AnimModeComboBox,
        AnimParamASpinBox,
        AnimParamBSpinBox,
        AnimParamCSpinBox,
        AnimParamDSpinBox,
    };

    void ActivateMatEditUI(bool Active);
    void RefreshMaterial();

private slots:
    void RefreshViewport();
    void SetActiveMaterial(int MatIndex);
    void SetActivePass(int PassIndex);
    void UpdateMaterial();
    void UpdateMaterial(int Value);
    void UpdateMaterial(int ValueA, int ValueB);
    void UpdateMaterial(double Value);
    void UpdateMaterial(bool Value);
    void UpdateMaterial(const QColor& Color);
    void UpdateMaterial(const CResourceEntry* Entry);
    void UpdateUI(int Value);
    void UpdateAnimParamUI(EUVAnimMode Mode);

    void Import();
    void ConvertToDDS();
    void ConvertToTXTR();
    void SetMeshPreview();
    void SetSpherePreview();
    void SetFlatPreview();
    void ClearColorChanged(const QColor& rkNewColor);
    void ToggleCameraMode();
    void ToggleGrid(bool Enabled);

signals:
    void Closed();
};

#endif // CMODELEDITORWINDOW_H
