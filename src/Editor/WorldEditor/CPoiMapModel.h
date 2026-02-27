#ifndef CPOIMAPMODEL_H
#define CPOIMAPMODEL_H

#include <Core/Resource/TResPtr.h>
#include <QAbstractTableModel>
#include <QList>
#include <QMap>

class CGameArea;
class CModelNode;
class CScriptNode;
class CPoiToWorld;
class CWorld;
class CWorldEditor;

class CPoiMapModel : public QAbstractListModel
{
    Q_OBJECT

    CWorldEditor *mpEditor;
    CGameArea *mpArea = nullptr;
    TResPtr<CPoiToWorld> mpPoiToWorld;

    QMap<const CScriptNode*, QList<CModelNode*>> mModelMap;

public:
    explicit CPoiMapModel(CWorldEditor *pEditor, QObject *pParent = nullptr);
    ~CPoiMapModel() override;

    QVariant headerData(int Section, Qt::Orientation Orientation, int Role) const override;
    int rowCount(const QModelIndex& rkParent = QModelIndex()) const override;
    QVariant data(const QModelIndex& rkIndex, int Role) const override;

    void AddPOI(const CScriptNode* pPOI);
    void AddMapping(const QModelIndex& rkIndex, CModelNode* pNode);
    void RemovePOI(const QModelIndex& rkIndex);
    void RemoveMapping(const QModelIndex& rkIndex, const CModelNode* pNode);
    bool IsPoiTracked(const CScriptNode* pPOI) const;
    bool IsModelMapped(const QModelIndex& rkIndex, const CModelNode* pNode) const;

    CScriptNode* PoiNodePointer(const QModelIndex& rkIndex) const;
    const QList<CModelNode*>& GetPoiMeshList(const QModelIndex& rkIndex);
    const QList<CModelNode*>& GetPoiMeshList(const CScriptNode* pPOI);

private slots:
    void OnMapChange(CWorld*, CGameArea *pArea);
};

#endif // CPOIMAPMODEL_H
