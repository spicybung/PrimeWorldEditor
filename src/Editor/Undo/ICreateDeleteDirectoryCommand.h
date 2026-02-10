#ifndef CCREATEDIRECTORYCOMMAND_H
#define CCREATEDIRECTORYCOMMAND_H

#include "Editor/Undo/IUndoCommand.h"
#include "Editor/CEditorApplication.h"
#include "Editor/ResourceBrowser/CResourceBrowser.h"
#include <Core/GameProject/CResourceStore.h>
#include <Core/GameProject/CVirtualDirectory.h>
#include <Common/Log.h>

#include <QCoreApplication>

class ICreateDeleteDirectoryCommand : public IUndoCommand
{
protected:
    CResourceStore *mpStore;
    TString mParentPath;
    TString mDirName;
    CVirtualDirectory *mpDir;

public:
    ICreateDeleteDirectoryCommand(const QString& rkText, CResourceStore *pStore, TString ParentPath, TString DirName)
        : IUndoCommand(rkText)
        , mpStore(pStore)
        , mParentPath(std::move(ParentPath))
        , mDirName(std::move(DirName))
        , mpDir(nullptr)
    {}

protected:
    void DoCreate()
    {
        CVirtualDirectory *pParent = mpStore->GetVirtualDirectory(mParentPath, false);

        if (pParent)
        {
            gpEdApp->ResourceBrowser()->DirectoryAboutToBeCreated( TO_QSTRING(mParentPath + mDirName) );
            mpDir = pParent->FindChildDirectory(mDirName, true);
            gpEdApp->ResourceBrowser()->DirectoryCreated(mpDir);
        }
    }

    void DoDelete()
    {
        if (mpDir && !mpDir->IsRoot())
        {
            if (mpDir->IsEmpty(true))
            {
                gpEdApp->ResourceBrowser()->DirectoryAboutToBeDeleted(mpDir);
                [[maybe_unused]] const bool DeleteSuccess = mpDir->Delete();
                ASSERT(DeleteSuccess);
                gpEdApp->ResourceBrowser()->DirectoryDeleted();

                mpDir = nullptr;
            }
            else
            {
                NLog::Debug("Directory delete failed, directory is not empty: {}", mParentPath + mDirName);
            }
        }
    }

    bool AffectsCleanState() const override { return false; }
};

class CCreateDirectoryCommand : public ICreateDeleteDirectoryCommand
{
public:
    CCreateDirectoryCommand(CResourceStore *pStore, TString ParentPath, TString DirName)
        : ICreateDeleteDirectoryCommand(QCoreApplication::translate("CCreateDirectoryCommand", "Create Directory"), pStore, std::move(ParentPath), std::move(DirName))
    {}

    void undo() override { DoDelete(); }
    void redo() override { DoCreate(); }
};

class CDeleteDirectoryCommand : public ICreateDeleteDirectoryCommand
{
public:
    CDeleteDirectoryCommand(CResourceStore *pStore, TString ParentPath, TString DirName)
        : ICreateDeleteDirectoryCommand(QCoreApplication::translate("CDeleteDirectoryCommand", "Delete Directory"), pStore, std::move(ParentPath), std::move(DirName))
    {
        mpDir = pStore->GetVirtualDirectory(mParentPath + mDirName, false);
        ASSERT(mpDir);
        ASSERT(!mpDir->IsRoot());
    }

    void undo() override { DoCreate(); }
    void redo() override { DoDelete(); }
};

#endif // CCREATEDIRECTORYCOMMAND_H
