#ifndef DIALOGSCRIPTSETTINGS_H
#define DIALOGSCRIPTSETTINGS_H

#include "scriptsettings.h"
#include "qfixedsizedialog.h"


namespace Ui {
class DialogScriptSettings;
}

class DialogScriptSettings : public QFixedSizeDialog
{
    Q_OBJECT

public:
    explicit DialogScriptSettings(ScriptSettings& ss, QWidget *parent = nullptr);
    ~DialogScriptSettings();

    void accept() override;

private:
    Ui::DialogScriptSettings *ui;
    ScriptSettings& _scriptSettings;
};

#endif // DIALOGSCRIPTSETTINGS_H
