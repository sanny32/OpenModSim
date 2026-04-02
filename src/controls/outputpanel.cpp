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

    // When the user clicks a different tab, move focus to the tab bar before
    // the outgoing tab widget is hidden. Without this, Qt transfers focus from
    // the focused child (e.g. the log list widget) to the first widget in the
    // global focus chain, which happens to be a child of the first MDI
    // sub-window, causing an unintended MDI tab switch.
    connect(ui->tabWidget->tabBar(), &QTabBar::tabBarClicked, this, [this](int index) {
        if (index != ui->tabWidget->currentIndex())
            ui->tabWidget->tabBar()->setFocus(Qt::OtherFocusReason);
    });
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
