#include "dialogscriptsettings.h"
#include "ui_dialogscriptsettings.h"

///
/// \brief DialogScriptSettings::DialogScriptSettings
/// \param ss
/// \param parent
///
DialogScriptSettings::DialogScriptSettings(ScriptSettings& ss, QWidget *parent)
    : QFixedSizeDialog(parent)
    , ui(new Ui::DialogScriptSettings)
    ,_scriptSettings(ss)
{
    ui->setupUi(this);
    ui->lineEditInterval->setInputRange(500, 10000);
    ui->lineEditInterval->setValue(ss.Interval);
    ui->comboBoxRunMode->setCurrentRunMode(ss.Mode);
    ui->checkBoxAutoComplete->setChecked(ss.UseAutoComplete);
    ui->checkBoxRunOnStartup->setChecked(ss.RunOnStartup);
}

///
/// \brief DialogScriptSettings::~DialogScriptSettings
///
DialogScriptSettings::~DialogScriptSettings()
{
    delete ui;
}

///
/// \brief DialogScriptSettings::accept
///
void DialogScriptSettings::accept()
{
    _scriptSettings.Mode = ui->comboBoxRunMode->currentRunMode();
    _scriptSettings.Interval = ui->lineEditInterval->value<int>();
    _scriptSettings.UseAutoComplete = ui->checkBoxAutoComplete->isChecked();
    _scriptSettings.RunOnStartup = ui->checkBoxRunOnStartup->isChecked();

    QFixedSizeDialog::accept();
}
