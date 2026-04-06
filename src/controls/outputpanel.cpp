#include <QTabBar>
#include "outputpanel.h"
#include "ui_outputpanel.h"
#include "applogoutput.h"
#include "consoleoutput.h"

///
/// \brief OutputPanel::OutputPanel
///
OutputPanel::OutputPanel(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::OutputPanel)
{
    ui->setupUi(this);

    connect(ui->appLog,    &AppLogOutput::collapse,  this, &OutputPanel::collapse);
    connect(ui->jsConsole, &ConsoleOutput::collapse, this, &OutputPanel::collapse);
}

///
/// \brief OutputPanel::~OutputPanel
///
OutputPanel::~OutputPanel()
{
    delete ui;
}

///
/// \brief OutputPanel::changeEvent
///
void OutputPanel::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        ui->retranslateUi(this);
    QWidget::changeEvent(event);
}

///
/// \brief OutputPanel::appLog
///
AppLogOutput* OutputPanel::appLog() const
{
    return ui->appLog;
}

///
/// \brief OutputPanel::jsConsole
///
ConsoleOutput* OutputPanel::jsConsole() const
{
    return ui->jsConsole;
}

///
/// \brief OutputPanel::switchToJsConsole
///
void OutputPanel::switchToJsConsole()
{
    ui->tabWidget->setCurrentWidget(ui->jsConsole);
}

