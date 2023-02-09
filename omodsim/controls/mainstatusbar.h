#ifndef MAINSTATUSBAR_H
#define MAINSTATUSBAR_H

#include <QLabel>
#include <QStatusBar>
#include "modbusmultiserver.h"

///
/// \brief The MainStatusBar class
///
class MainStatusBar : public QStatusBar
{
    Q_OBJECT
public:
    explicit MainStatusBar(const ModbusMultiServer& server, QWidget* parent = nullptr);

private:
    QList<QLabel*> _labels;
};

#endif // MAINSTATUSBAR_H
