#ifndef JSHELPCONTROL_H
#define JSHELPCONTROL_H

#include <QTextBrowser>

class JSHelpControl : public QTextBrowser
{
    Q_OBJECT
public:
    JSHelpControl(QWidget* parent = nullptr);
};

#endif // JSHELPCONTROL_H
