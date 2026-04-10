#include <QEvent>
#include <QDesktopServices>
#include <QMdiSubWindow>
#include <QUrl>
#include "apppreferences.h"
#include "mainstatusbar.h"
#include "serialportutils.h"

///
/// \brief MainStatusBar::MainStatusBar
/// \param parent
///
MainStatusBar::MainStatusBar(const ModbusMultiServer& server, QWidget* parent)
    : QStatusBar(parent)
    , _server(server)
    , _deviceIdsLabel(new QLabel(this))
    , _clientCountLabel(new QLabel(this))
    , _errorSimLabel(new QLabel(this))
    , _requestCountLabel(new QLabel(this))
    , _updateChecker(new UpdateChecker(this))
{
    _requestCountLabel->setContentsMargins(6, 0, 6, 0);
    insertPermanentWidget(0, _requestCountLabel);
    updateRequestCountInfo();

    _clientCountLabel->setContentsMargins(6, 0, 6, 0);
    insertPermanentWidget(1, _clientCountLabel);
    updateClientCountInfo();

    _deviceIdsLabel->setContentsMargins(6, 0, 6, 0);
    addPermanentWidget(_deviceIdsLabel);
    updateDeviceIdsInfo();

    _errorSimLabel->setTextFormat(Qt::RichText);
    _errorSimLabel->setContentsMargins(6, 0, 6, 0);
    _errorSimLabel->setVisible(false);
    addPermanentWidget(_errorSimLabel);

    connect(&server, &ModbusMultiServer::definitionsChanged, this, [this](const ModbusDefinitions& defs)
    {
        _errorSimulations = defs.ErrorSimulations;
        updateErrorSimInfo();
    });

    _bellButton = new QToolButton(this);
    _bellButton->setIcon(QIcon(":/res/icon-bell.svg"));
    _bellButton->setAutoRaise(true);
    _bellButton->setToolTip(tr("No updates available"));
    _bellButton->setCursor(Qt::PointingHandCursor);
    _bellButton->setFixedSize(22, 22);
    _bellButton->setIconSize(QSize(16, 16));
    addPermanentWidget(_bellButton);

    connect(_bellButton, &QToolButton::clicked, this, [this]()
    {
        if(_updateChecker->hasNewVersion())
        {
            QDesktopServices::openUrl(QUrl(_updateChecker->releaseUrl()));
        }
    });

    connect(_updateChecker, &UpdateChecker::newVersionAvailable,
            this, &MainStatusBar::onNewVersionAvailable);

    connect(&server, &ModbusMultiServer::deviceIdsChanged, this, [this](const QList<int>& deviceIds)
    {
        _deviceIds = deviceIds;
        updateDeviceIdsInfo();
    });

    connect(&server, &ModbusMultiServer::clientConnected, this, [this](const ModbusClientInfo&)
    {
        updateClientCountInfo();
    });

    connect(&server, &ModbusMultiServer::clientDisconnected, this, [this](const ModbusClientInfo&)
    {
        updateClientCountInfo();
    });

    connect(&server, &ModbusMultiServer::connected, this, [&](const ConnectionDetails& cd)
    {
        auto label = new QLabel(this);
        label->setTextFormat(Qt::RichText);
        label->setContentsMargins(6, 0, 6, 0);
        label->setProperty("ConnectionDetails", QVariant::fromValue(cd));

        updateConnectionInfo(label, cd);

        _labels.append(label);
        insertWidget(_labels.size() - 1, label);
        updateClientCountInfo();
    });

    connect(&server, &ModbusMultiServer::disconnected, this, [&](const ConnectionDetails& cd)
    {
        for(auto&& label : _labels)
        {
            if(cd == label->property("ConnectionDetails").value<ConnectionDetails>())
            {
                removeWidget(label);
                _labels.removeOne(label);

                break;
            }
        }

        if(_labels.isEmpty())
        {
            _requestCount  = 0;
            _responseCount = 0;
            updateRequestCountInfo();
        }

        updateClientCountInfo();
    });

    connect(&server, &ModbusMultiServer::request, this, [this](const QSharedPointer<const ModbusMessage>&)
    {
        _requestCount++;
        updateRequestCountInfo();
    });

    connect(&server, &ModbusMultiServer::response, this, [this](const QSharedPointer<const ModbusMessage>&,
                                                                  const QSharedPointer<const ModbusMessage>&)
    {
        _responseCount++;
        updateRequestCountInfo();
    });

    // Initial check after a short delay
    if(AppPreferences::instance().checkForUpdates())
        QTimer::singleShot(3000, _updateChecker, &UpdateChecker::checkForUpdates);
    else
        _bellButton->setVisible(false);
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

        if(_updateChecker->hasNewVersion())
            _bellButton->setToolTip(tr("New version %1 is available. Click to download.").arg(_updateChecker->latestVersion()));
        else
            _bellButton->setToolTip(tr("No updates available"));

        updateDeviceIdsInfo();
        updateErrorSimInfo();
        updateClientCountInfo();
    }

    QStatusBar::changeEvent(event);
}

///
/// \brief MainStatusBar::updateDeviceIdsInfo
///
void MainStatusBar::updateDeviceIdsInfo()
{
    QString values(QChar(0x2014));
    if(!_deviceIds.isEmpty())
    {
        QStringList textIds;
        textIds.reserve(_deviceIds.size());
        for(auto&& deviceId : _deviceIds)
            textIds.append(QString::number(deviceId));

        values = textIds.join(", ");
    }

    _deviceIdsLabel->setText(tr("Unit Identifiers: %1").arg(values));
}

///
/// \brief MainStatusBar::updateErrorSimInfo
///
void MainStatusBar::updateErrorSimInfo()
{
    const QString warn = QStringLiteral("<span style='color:#f97316; font-size:10px;'>&#9679;</span>&nbsp;");
    QStringList flags;

    if(_errorSimulations.noResponse())
        flags << tr("No Resp");
    if(_errorSimulations.responseIncorrectId())
        flags << tr("Bad ID");
    if(_errorSimulations.responseIllegalFunction())
        flags << tr("Ill Func");
    if(_errorSimulations.responseDeviceBusy())
        flags << tr("Dev Busy");
    if(_errorSimulations.responseIncorrectCrc())
        flags << tr("Bad CRC");
    if(_errorSimulations.responseDelay())
        flags << tr("Delay %1ms").arg(_errorSimulations.responseDelayTime());
    if(_errorSimulations.responseRandomDelay())
        flags << tr("Rnd Delay %1ms").arg(_errorSimulations.responseRandomDelayUpToTime());

    if(flags.isEmpty())
    {
        _errorSimLabel->setVisible(false);
    }
    else
    {
        _errorSimLabel->setText(warn + flags.join(", "));
        _errorSimLabel->setVisible(true);
    }
}

///
/// \brief MainStatusBar::updateClientCountInfo
///
void MainStatusBar::updateClientCountInfo()
{
    _clientCountLabel->setText(tr("Clients: %1").arg(_server.connectedClientCount()));
}

///
/// \brief MainStatusBar::updateConnectionInfo
/// \param label
/// \param cd
///
void MainStatusBar::updateConnectionInfo(QLabel* label, const ConnectionDetails& cd)
{
    const QString dot = QStringLiteral("<span style='color:#22c55e; font-size:10px;'>&#9679;</span>&nbsp;");
    switch(cd.Type)
    {
        case ConnectionType::Tcp:
            label->setText(dot + tr("Modbus/TCP Srv %1:%2").arg(cd.TcpParams.IPAddress, QString::number(cd.TcpParams.ServicePort)));
        break;

        case ConnectionType::Serial:
            label->setText(dot + tr("Port %1:%2:%3:%4:%5").arg(cd.SerialParams.PortName,
                                                                QString::number(cd.SerialParams.BaudRate),
                                                                QString::number(cd.SerialParams.WordLength),
                                                                Parity_toString(cd.SerialParams.Parity),
                                                                QString::number(cd.SerialParams.StopBits)));
        break;
    }
}

///
/// \brief MainStatusBar::setCheckForUpdates
/// \param enabled
///
void MainStatusBar::setCheckForUpdates(bool enabled)
{
    _bellButton->setVisible(enabled);
    _updateChecker->setEnabled(enabled);
}

///
/// \brief MainStatusBar::updateRequestCountInfo
///
void MainStatusBar::updateRequestCountInfo()
{
    _requestCountLabel->setText(tr("Req: %1  Resp: %2").arg(_requestCount).arg(_responseCount));
}

///
/// \brief MainStatusBar::onNewVersionAvailable
/// \param version
/// \param url
///
void MainStatusBar::onNewVersionAvailable(const QString& version, const QString& url)
{
    Q_UNUSED(url);
    _bellButton->setIcon(QIcon(":/res/icon-bell-dot.svg"));
    _bellButton->setToolTip(tr("New version %1 is available. Click to download.").arg(version));
}


