#ifndef CUIRELAY_H
#define CUIRELAY_H

#include <Core/IUIRelay.h>
#include "Editor/CEditorApplication.h"
#include "Editor/UICommon.h"
#include "Editor/WorldEditor/CWorldEditor.h"

#include <QThread>

class CUIRelay : public QObject, public IUIRelay
{
    Q_OBJECT

    static Qt::ConnectionType GetConnectionType()
    {
        const bool IsUIThread = (QThread::currentThread() == gpEdApp->thread());
        return IsUIThread ? Qt::DirectConnection : Qt::BlockingQueuedConnection;
    }

public:
    explicit CUIRelay(QObject *pParent = nullptr)
        : QObject(pParent)
    {}

    // Note: All function calls should be deferred with QMetaObject::invokeMethod to ensure
    // that they run on the UI thread instead of whatever thread we happen to be on.
    void ShowMessageBox(const std::string& rkInfoBoxTitle, const std::string& rkMessage) override
    {
        QMetaObject::invokeMethod(this, &CUIRelay::MessageBoxSlot, GetConnectionType(),
                                  TO_QSTRING(rkInfoBoxTitle), TO_QSTRING(rkMessage));
    }

    void ShowMessageBoxAsync(const std::string&rkInfoBoxTitle, const std::string& rkMessage) override
    {
        QMetaObject::invokeMethod(this, &CUIRelay::MessageBoxSlot, Qt::QueuedConnection,
                                  TO_QSTRING(rkInfoBoxTitle), TO_QSTRING(rkMessage));
    }

    bool AskYesNoQuestion(const std::string& rkInfoBoxTitle, const std::string& rkQuestion) override
    {
        bool RetVal;
        QMetaObject::invokeMethod(this, &CUIRelay::AskYesNoQuestionSlot, GetConnectionType(), qReturnArg(RetVal),
                                  TO_QSTRING(rkInfoBoxTitle), TO_QSTRING(rkQuestion));
        return RetVal;
    }

    OpenProjectResult OpenProject(const std::string& kPath = {}) override
    {
        bool RetVal;
        QMetaObject::invokeMethod(this, &CUIRelay::OpenProjectSlot, GetConnectionType(), qReturnArg(RetVal),
                                  TO_QSTRING(kPath));
        return {
            .success = RetVal,
            .project = gpEdApp->ActiveProject(),
        };
    }

private slots:
    void MessageBoxSlot(const QString& rkInfoBoxTitle, const QString& rkMessage)
    {
        UICommon::InfoMsg(gpEdApp->WorldEditor(), rkInfoBoxTitle, rkMessage);
    }

    bool AskYesNoQuestionSlot(const QString& rkInfoBoxTitle, const QString& rkQuestion)
    {
        return UICommon::YesNoQuestion(gpEdApp->WorldEditor(), rkInfoBoxTitle, rkQuestion);
    }

    bool OpenProjectSlot(const QString& kPath)
    {
        return !kPath.isEmpty() ? gpEdApp->OpenProject(kPath) : UICommon::OpenProject();
    }
};

#endif // CUIRELAY_H
