#include "Editor/ResourceBrowser/CResourceDelegate.h"

#include "Editor/CFileNameValidator.h"
#include "Editor/UICommon.h"
#include "Editor/ResourceBrowser/CResourceBrowser.h"
#include "Editor/ResourceBrowser/CResourceProxyModel.h"
#include "Editor/ResourceBrowser/CResourceTableModel.h"
#include <Core/GameProject/CResourceEntry.h>
#include <Core/GameProject/CVirtualDirectory.h>
#include <Core/Resource/CResTypeInfo.h>
#include <Common/Macros.h>

#include <QLineEdit>
#include <QPainter>

#include <algorithm>

// Geometry Info
struct SResDelegateGeometryInfo
{
    QRect InnerRect;
    QRect IconRect;
    QRect NameStringRect;
    QRect InfoStringRect;
};
static SResDelegateGeometryInfo GetGeometryInfo(const SDelegateFontInfo& rkFontInfo, const QStyleOptionViewItem& rkOption, bool IsDirectory)
{
    SResDelegateGeometryInfo Info;

    // Calculate inner rect
    const int Margin = rkFontInfo.Margin;
    Info.InnerRect = rkOption.rect.adjusted(Margin, Margin, -Margin, -Margin);

    // Calculate icon
    const int IdealIconSize = CResourceBrowserDelegate::skIconSize;
    const int IconSize = std::min(IdealIconSize, Info.InnerRect.height());
    const int IconX = Info.InnerRect.left() + ((IdealIconSize - IconSize) / 2);
    const int IconY = Info.InnerRect.top() + ((Info.InnerRect.height() - IconSize) / 2);
    Info.IconRect = QRect(IconX, IconY, IconSize, IconSize);

    // Calculate name string
    const int NameX = Info.InnerRect.left() + IdealIconSize + (rkFontInfo.Spacing * 2);
    int NameY = Info.InnerRect.top();
    const int NameSizeX = Info.InnerRect.right() - NameX;
    const int NameSizeY = rkFontInfo.NameFontMetrics.height();

    // Adjust Y for directories to center it in the rect
    if (IsDirectory)
    {
        NameY += (Info.InnerRect.height() - NameSizeY) / 2;
    }

    Info.NameStringRect = QRect(NameX, NameY, NameSizeX, NameSizeY);

    // Calculate info string
    if (!IsDirectory)
    {
        const int InfoX = NameX;
        const int InfoY = NameY + NameSizeY + rkFontInfo.Spacing;
        const int InfoSizeX = NameSizeX;
        const int InfoSizeY = rkFontInfo.InfoFontMetrics.height();
        Info.InfoStringRect = QRect(InfoX, InfoY, InfoSizeX, InfoSizeY);
    }

    return Info;
}

// Delegate implementation
QSize CResourceBrowserDelegate::sizeHint(const QStyleOptionViewItem& rkOption, const QModelIndex&) const
{
    // Get string info
    const SDelegateFontInfo Info = GetFontInfo(rkOption);

    // Calculate height
    const int Height = (Info.Margin * 2) + Info.NameFontMetrics.height() + Info.Spacing + Info.InfoFontMetrics.height();
    return QSize(0, Height);
}

void CResourceBrowserDelegate::paint(QPainter *pPainter, const QStyleOptionViewItem& rkOption, const QModelIndex& rkIndex) const
{
    // Get resource entry
    const CResourceEntry* pEntry = GetIndexEntry(rkIndex);

    // Initialize
    const SDelegateFontInfo FontInfo = GetFontInfo(rkOption);
    const SResDelegateGeometryInfo GeomInfo = GetGeometryInfo(FontInfo, rkOption, pEntry == nullptr);

    // Draw icon
    const QVariant IconVariant = rkIndex.model()->data(rkIndex, Qt::DecorationRole);

    if (IconVariant.isValid())
    {
        const auto Icon = IconVariant.value<QIcon>();
        Icon.paint(pPainter, GeomInfo.IconRect);
    }

    // Draw resource name
    const QString ResName = rkIndex.model()->data(rkIndex, Qt::DisplayRole).toString();
    const QString ElidedName = FontInfo.NameFontMetrics.elidedText(ResName, Qt::ElideRight, GeomInfo.NameStringRect.width());

    pPainter->setFont(FontInfo.NameFont);
    pPainter->setPen(FontInfo.NamePen);
    pPainter->drawText(GeomInfo.NameStringRect, ElidedName);

    // Draw resource info string
    if (pEntry)
    {
        QString ResInfo = TO_QSTRING(pEntry->TypeInfo()->TypeName());

        if (mDisplayAssetIDs)
            ResInfo.prepend(TO_QSTRING(pEntry->ID().ToString()) + QStringLiteral(" | "));

        const QString ElidedResInfo = FontInfo.InfoFontMetrics.elidedText(ResInfo, Qt::ElideRight, GeomInfo.InfoStringRect.width());

        pPainter->setFont(FontInfo.InfoFont);
        pPainter->setPen(FontInfo.InfoPen);
        pPainter->drawText(GeomInfo.InfoStringRect, ElidedResInfo);
    }
}

// Editor
QWidget* CResourceBrowserDelegate::createEditor(QWidget *pParent, const QStyleOptionViewItem&, const QModelIndex& rkIndex) const
{
    bool IsDirectory = (GetIndexDirectory(rkIndex) != nullptr);

    auto* pLineEdit = new QLineEdit(pParent);
    pLineEdit->setValidator(new CFileNameValidator(IsDirectory, pLineEdit));

    // Set the max length to 150. Limit should be 255 but FileUtil::MoveFile doesn't
    // seem to want to work with filenames that long. Not sure why.
    pLineEdit->setMaxLength(150);

    return pLineEdit;
}

void CResourceBrowserDelegate::setEditorData(QWidget *pEditor, const QModelIndex& rkIndex) const
{
    auto* pLineEdit = static_cast<QLineEdit*>(pEditor);

    if (const auto* pEntry = GetIndexEntry(rkIndex))
        pLineEdit->setText(TO_QSTRING(pEntry->Name()));
    else
        pLineEdit->setText(TO_QSTRING(GetIndexDirectory(rkIndex)->Name()));
}

void CResourceBrowserDelegate::setModelData(QWidget *pEditor, QAbstractItemModel *, const QModelIndex& rkIndex) const
{
    auto* pLineEdit = static_cast<QLineEdit*>(pEditor);
    QString NewName = pLineEdit->text();
    pLineEdit->validator()->fixup(NewName);

    if (!NewName.isEmpty())
    {
        if (auto* pEntry = GetIndexEntry(rkIndex))
            gpEdApp->ResourceBrowser()->RenameResource(pEntry, TO_TSTRING(NewName));
        else
            gpEdApp->ResourceBrowser()->RenameDirectory(GetIndexDirectory(rkIndex), TO_TSTRING(NewName));
    }
}

void CResourceBrowserDelegate::updateEditorGeometry(QWidget *pEditor, const QStyleOptionViewItem& rkOption, const QModelIndex& rkIndex) const
{
    // Check if this is a directory
    const bool IsDir = GetIndexEntry(rkIndex) == nullptr;

    // Get rect
    const SDelegateFontInfo FontInfo = GetFontInfo(rkOption);
    const SResDelegateGeometryInfo GeomInfo = GetGeometryInfo(FontInfo, rkOption, IsDir);

    // Set geometry; make it a little bit better than the name string rect to give the user more space
    const QRect WidgetRect = GeomInfo.NameStringRect.adjusted(-3, -3, 3, 3);
    pEditor->setGeometry(WidgetRect);
}

CResourceEntry* CResourceBrowserDelegate::GetIndexEntry(const QModelIndex& rkIndex) const
{
    const auto* pkProxy = qobject_cast<const CResourceProxyModel*>(rkIndex.model());
    ASSERT(pkProxy != nullptr);

    const auto* pkModel = qobject_cast<const CResourceTableModel*>(pkProxy->sourceModel());
    ASSERT(pkModel != nullptr);

    const auto SourceIndex = pkProxy->mapToSource(rkIndex);
    return pkModel->IndexEntry(SourceIndex);
}

CVirtualDirectory* CResourceBrowserDelegate::GetIndexDirectory(const QModelIndex& rkIndex) const
{
    const auto* pkProxy = qobject_cast<const CResourceProxyModel*>(rkIndex.model());
    ASSERT(pkProxy != nullptr);

    const auto* pkModel = qobject_cast<const CResourceTableModel*>(pkProxy->sourceModel());
    ASSERT(pkModel != nullptr);

    const auto SourceIndex = pkProxy->mapToSource(rkIndex);
    return pkModel->IndexDirectory(SourceIndex);
}
