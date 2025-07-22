#ifndef MENUCONNECT_H
#define MENUCONNECT_H

#include <QMenu>
#include "enums.h"
#include "modbusmultiserver.h"

class MenuConnect : public QMenu
{
    Q_OBJECT
public:
    enum MenuType
    {
        ConnectMenu = 0,
        DisconnectMenu
    };

    explicit MenuConnect(MenuType type, ModbusMultiServer& server, QWidget *parent = nullptr);

    bool canConnect(const ConnectionDetails& cd);
    void updateConnectionDetails(const QList<ConnectionDetails>& conns);

    friend QSettings& operator <<(QSettings& out, const MenuConnect* menu);
    friend QSettings& operator >>(QSettings& in, MenuConnect* menu);

signals:
    void connectAction(ConnectionDetails& cd);
    void disconnectAction(ConnectionType type, const QString& port);

protected:
    void changeEvent(QEvent* event) override;

private:
    void addAction(const QString& text, ConnectionType type, const QString& port);

private:
    MenuType _menuType;
    ModbusMultiServer& _mbMultiServer;
    QMap<QAction*, ConnectionDetails> _connectionDetailsMap;
};

#endif // MENUCONNECT_H
