#ifndef CLOGDIALOG_H
#define CLOGDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui {
class CLogDialog;
}

class CLogDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CLogDialog(QWidget *pParent);
    ~CLogDialog() override;

private:
    std::unique_ptr<Ui::CLogDialog> ui;
};

#endif // CLOGDIALOG_H
