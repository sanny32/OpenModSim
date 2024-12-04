#include <QEvent>
#include <QMdiSubWindow>
#include "mainstatusbar.h"

///
/// \brief Parity_toString
/// \param parity
/// \return
///
QString Parity_toString(QSerialPort::Parity parity)
{
    switch(parity)
    {
        case QSerialPort::NoParity:
        return MainStatusBar::tr("NONE");

        case QSerialPort::EvenParity:
        return MainStatusBar::tr("EVEN");

        case QSerialPort::OddParity:
        return MainStatusBar::tr("ODD");

        case QSerialPort::SpaceParity:
        return MainStatusBar::tr("SPACE");

        case QSerialPort::MarkParity:
        return MainStatusBar::tr("MARK");

        default:
        break;
    }

    return QString();
}

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
        label->setFrameShadow(QFrame::Sunken);
        label->setFrameShape(QFrame::Panel);
        label->setMinimumWidth(120);
        label->setProperty("ConnectionDetails", QVariant::fromValue(cd));

        updateConnectionInfo(label, cd);

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

///
/// \brief MainStatusBar::~MainStatusBar
///
MainStatusBar::~MainStatusBar()
{
    for(auto&& label : _labels)
        delete label;
}

///
/// \brief MainStatusBar::changeEvent
/// \param event
///
void MainStatusBar::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        for(auto&& label : _labels)
        {
            const auto cd = label->property("ConnectionDetails").value<ConnectionDetails>();
            updateConnectionInfo(label, cd);
        }
    }

    QStatusBar::changeEvent(event);
}

///
/// \brief MainStatusBar::updateConnectionInfo
/// \param label
/// \param cd
///
void MainStatusBar::updateConnectionInfo(QLabel* label, const ConnectionDetails& cd)
{
    switch(cd.Type)
    {
        case ConnectionType::Tcp:
            label->setText(QString(tr("TCP/IP %1:%2")).arg(cd.TcpParams.IPAddress, QString::number(cd.TcpParams.ServicePort)));
        break;

        case ConnectionType::Serial:
            label->setText(QString(tr("Port %1:%2:%3:%4:%5  ")).arg(cd.SerialParams.PortName,
                                                                    QString::number(cd.SerialParams.BaudRate),
                                                                    QString::number(cd.SerialParams.WordLength),
                                                                    Parity_toString(cd.SerialParams.Parity),
                                                                    QString::number(cd.SerialParams.StopBits)));
        break;
    }
}
