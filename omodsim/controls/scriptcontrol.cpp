#include "scriptcontrol.h"
#include "ui_scriptcontrol.h"

///
/// \brief ScriptControl::ScriptControl
/// \param parent
///
ScriptControl::ScriptControl(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::ScriptControl)
{
    ui->setupUi(this);

    _toolBar = new QToolBar(this);
    ((QVBoxLayout*)layout())->insertWidget(0, _toolBar);
}

///
/// \brief ScriptControl::~ScriptControl
///
ScriptControl::~ScriptControl()
{
    delete ui;
}
