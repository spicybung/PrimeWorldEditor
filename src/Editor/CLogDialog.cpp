#include "Editor/CLogDialog.h"
#include "ui_CLogDialog.h"

#include <Common/Log.h>
#include "Editor/UICommon.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/qt_sinks.h>

CLogDialog::CLogDialog(QWidget *pParent)
    : QDialog(pParent)
    , ui(std::make_unique<Ui::CLogDialog>())
{
    ui->setupUi(this);
    connect(ui->CloseButton, &QPushButton::clicked, this, &CLogDialog::close);
    connect(ui->ClearLogButton, &QPushButton::clicked, [this] {
        ui->LogTextEdit->clear();
    });

    constexpr int max_lines = 300;
    NLog::Get()->sinks().push_back(std::make_shared<spdlog::sinks::qt_color_sink_mt>(ui->LogTextEdit, max_lines, false, true));
}

CLogDialog::~CLogDialog()
{
    NLog::Get()->sinks().pop_back();
}
