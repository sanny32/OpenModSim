#include <QMdiSubWindow>
#include "mainstatusbar.h"

///
/// \brief MainStatusBar::MainStatusBar
/// \param parent
///
MainStatusBar::MainStatusBar(const ModbusMultiServer& server, QWidget* parent)
    : QStatusBar(parent)
{
    connect(&server, &ModbusMultiServer::connected, this, [&](const ConnectionDetails& cd)
    {
        auto label = new QLabel(this);
        label->setText(QString(tr("Polls: %1")).arg(0));
        label->setFrameShadow(QFrame::Sunken);
        label->setFrameShape(QFrame::Panel);
        label->setMinimumWidth(120);
        label->setProperty("ConnectionDetails", QVariant::fromValue(cd));

        switch(cd.Type)
        {
            case ConnectionType::Tcp:
                label->setText(QString(tr("Modbus/TCP Srv: %1")).arg(cd.TcpParams.ServicePort));
            break;

            case ConnectionType::Serial:
                label->setText(QString(tr("Port %1: %2")).arg(cd.SerialParams.PortName, QString::number(cd.SerialParams.BaudRate)));
            break;
        }

        _labels.append(label);
        addPermanentWidget(label);
    });

    connect(&server, &ModbusMultiServer::disconnected, this, [&](const ConnectionDetails& cd)
    {
        for(auto&& label : _labels)
        {
            if(cd == label->property("ConnectionDetails").value<ConnectionDetails>())
            {
                _labels.removeOne(label);
                removeWidget(label);
                delete label;

                break;
            }
        }
    });
}
