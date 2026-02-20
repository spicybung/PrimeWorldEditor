#ifndef IUIRELAY_H
#define IUIRELAY_H

#include <string>

class CGameProject;

class IUIRelay
{
public:
    struct OpenProjectResult
    {
        bool success{};
        CGameProject* project{};
    };

    virtual ~IUIRelay() = default;
    virtual void ShowMessageBox(const std::string& rkInfoBoxTitle, const std::string& rkMessage) = 0;
    virtual void ShowMessageBoxAsync(const std::string& rkInfoBoxTitle, const std::string& rkMessage) = 0;
    virtual bool AskYesNoQuestion(const std::string& rkInfoBoxTitle, const std::string& rkQuestion) = 0;
    virtual OpenProjectResult OpenProject(const std::string& kPath = {}) = 0;
};

void SetUIRelay(IUIRelay* relay);
IUIRelay* GetUIRelay();

#endif // IUIRELAY_H
