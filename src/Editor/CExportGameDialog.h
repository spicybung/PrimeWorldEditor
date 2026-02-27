#ifndef CEXPORTGAMEDIALOG_H
#define CEXPORTGAMEDIALOG_H

#include <Common/EGame.h>
#include <Core/GameProject/CGameExporter.h>

#include <QDialog>
#include <QString>

#include <memory>

namespace nod {
class DiscBase;
}

namespace Ui {
class CExportGameDialog;
}

class CGameExporter;

class CExportGameDialog : public QDialog
{
    Q_OBJECT

    std::unique_ptr<Ui::CExportGameDialog> mpUI;
    std::unique_ptr<nod::DiscBase> mpDisc;
    std::unique_ptr<CGameExporter> mpExporter;

    TString mGameTitle;
    TString mGameID;

    // Build Info
    EDiscType mDiscType{EDiscType::Normal};
    EGame mGame{EGame::Invalid};
    ERegion mRegion{ERegion::Unknown};
    float mBuildVer = 0.0f;
    bool mWiiFrontend = false;

    bool mExportSuccess = false;
    QString mNewProjectPath;

    void InitUI(QString ExportDir);
    bool ValidateGame();
    bool RequestWiiPortGame();
    float FindBuildVersion() const;

    // Disc Tree
    void RecursiveAddToTree(const nod::Node* pkNode, class QTreeWidgetItem* pParent);

public:
    explicit CExportGameDialog(const QString& rkIsoPath, const QString& rkExportDir, QWidget *pParent = nullptr);
    ~CExportGameDialog() override;

    // Accessors
    bool HasValidDisc() const    { return mpDisc != nullptr; }
    bool ExportSucceeded() const { return mExportSuccess; }
    const QString& ProjectPath() const  { return mNewProjectPath; }

private slots:
    void BrowseOutputDirectory();
    void BrowseAssetNameMap();
    void BrowseGameEditorInfo();
    void Export();
};

#endif // CEXPORTGAMEDIALOG_H
