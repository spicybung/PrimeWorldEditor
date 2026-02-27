#ifndef CVIRTUALDIRECTORYTREEVIEW_H
#define CVIRTUALDIRECTORYTREEVIEW_H

#include <QTreeView>

class CVirtualDirectory;
class CVirtualDirectoryModel;

class CVirtualDirectoryTreeView : public QTreeView
{
    Q_OBJECT

    CVirtualDirectoryModel *mpModel = nullptr;
    bool mTransferSelectionPostMove = false;

public:
    explicit CVirtualDirectoryTreeView(QWidget *pParent = nullptr);
    ~CVirtualDirectoryTreeView() override;

    void dragEnterEvent(QDragEnterEvent *pEvent) override;
    void setModel(QAbstractItemModel *pModel) override;

private slots:
    void OnDirectoryAboutToBeMoved(const CVirtualDirectory *pDir);
    void OnDirectoryMoved(const CVirtualDirectory *pDir);
};

#endif // CVIRTUALDIRECTORYTREEVIEW_H
