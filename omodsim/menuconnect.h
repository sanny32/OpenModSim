#ifndef MENUCONNECT_H
#define MENUCONNECT_H

#include <QMenu>
#include "enums.h"

class MenuConnect : public QMenu
{
    Q_OBJECT
public:
    MenuConnect(QWidget *parent = nullptr);

signals:
    void connectAction(ConnectionType type, const QString& port);
};

#endif // MENUCONNECT_H
