#ifndef SCRIPTCONTROL_H
#define SCRIPTCONTROL_H

#include <QToolBar>
#include <QPlainTextEdit>

namespace Ui {
class ScriptControl;
}

///
/// \brief The ScriptControl class
///
class ScriptControl : public QWidget
{
    Q_OBJECT

public:
    explicit ScriptControl(QWidget *parent = nullptr);
    ~ScriptControl();

private:
    Ui::ScriptControl *ui;
    QToolBar* _toolBar;
};

#endif // SCRIPTCONTROL_H
