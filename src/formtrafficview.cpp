#include <QPainter>
#include <QPalette>
#include <QDateTime>
#include <QHelpEngine>
#include <QHelpContentWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QSizePolicy>
#include <QCheckBox>
#include <QSignalBlocker>
#include <QAbstractEventDispatcher>
#include <QFileDialog>
#include <QMessageBox>
#include "mainwindow.h"
#include "modbusmessages.h"
#include "serialportutils.h"
#include "formtrafficview.h"
#include "ui_formtrafficview.h"

///
/// \brief FormTrafficView::FormTrafficView
/// \param num
/// \param parent
///
FormTrafficView::FormTrafficView(int id, ModbusMultiServer& server, MainWindow* parent)
    : QWidget(parent)
    , ui(new Ui::FormTrafficView)
    , _formId(id)
    , _mbMultiServer(server)
{
    Q_ASSERT(parent != nullptr);

    ui->setupUi(this);

    setWindowTitle(QString("Traffic%1").arg(_formId));
    setWindowIcon(QIcon(":/res/actionShowTraffic.png"));
    setupToolbarActions();
    setupFilterControls();
    setupToolbarLayout();
    initializeDisplayDefinition();

    ui->outputWidget->setFocus();

    setLogViewState(server.isConnected() ? LogViewState::Running : LogViewState::Unknown);

    setupServerConnections();

    auto dispatcher = QAbstractEventDispatcher::instance();
    connect(dispatcher, &QAbstractEventDispatcher::awake, this, &FormTrafficView::on_awake);

    ui->outputWidget->setup(_displayDefinition);
    updateSourceFilter();
    on_awake();
}

///
/// \brief FormTrafficView::~FormTrafficView
///
FormTrafficView::~FormTrafficView()
{
    delete ui;
}

///
/// \brief FormTrafficView::saveSettings
///
void FormTrafficView::saveSettings(QSettings& out) const
{
    out << const_cast<FormTrafficView*>(this);
}

///
/// \brief FormTrafficView::loadSettings
///
void FormTrafficView::loadSettings(QSettings& in)
{
    in >> this;
}

///
/// \brief FormTrafficView::saveXml
///
void FormTrafficView::saveXml(QXmlStreamWriter& xml) const
{
    xml << const_cast<FormTrafficView*>(this);
}

///
/// \brief FormTrafficView::loadXml
///
void FormTrafficView::loadXml(QXmlStreamReader& xml)
{
    xml >> this;
}

///
/// \brief FormTrafficView::on_awake
///
void FormTrafficView::on_awake()
{
    ui->actionExportTrafficLog->setEnabled(!ui->outputWidget->isLogEmpty());
}

///
/// \brief FormTrafficView::changeEvent
/// \param e
///
void FormTrafficView::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    QWidget::changeEvent(e);
}

///
/// \brief FormTrafficView::closeEvent
/// \param event
///
void FormTrafficView::closeEvent(QCloseEvent* event)
{
    emit closing();
    QWidget::closeEvent(event);
}

///
/// \brief FormTrafficView::mouseDoubleClickEvent
/// \param event
/// \return
///
///
/// \brief FormTrafficView::displayDefinition
/// \return
///
TrafficViewDefinitions FormTrafficView::displayDefinition() const
{
    TrafficViewDefinitions dd = _displayDefinition;
    dd.FormName = windowTitle();
    if (_unitIdFilter)
        dd.UnitFilter = static_cast<quint8>(_unitIdFilter->value());
    if (_funcCodeFilter)
        dd.FunctionCodeFilter = static_cast<qint16>(_funcCodeFilter->currentData().toInt());
    dd.LogViewLimit = ui->outputWidget->logViewLimit();
    if (_exceptionsFilter)
        dd.ExceptionsOnly = _exceptionsFilter->isChecked();
    dd.normalize();
    return dd;
}

///
/// \brief FormTrafficView::setDisplayDefinition
/// \param dd
///
void FormTrafficView::setDisplayDefinition(const TrafficViewDefinitions& dd)
{
    TrafficViewDefinitions next = dd;
    if(!next.FormName.isEmpty())
        setWindowTitle(next.FormName);
    else
        next.FormName = windowTitle();

    next.normalize();
    _displayDefinition = next;

    if (_unitIdFilter) {
        QSignalBlocker b(_unitIdFilter);
        _unitIdFilter->setValue(_displayDefinition.UnitFilter);
    }
    if (_funcCodeFilter) {
        QSignalBlocker b(_funcCodeFilter);
        int idx = _funcCodeFilter->findData(_displayDefinition.FunctionCodeFilter);
        if (idx < 0)
            idx = 0;
        _funcCodeFilter->setCurrentIndex(idx);
    }
    if (_rowLimitCombo) {
        QSignalBlocker b(_rowLimitCombo);
        int idx = _rowLimitCombo->findData(_displayDefinition.LogViewLimit);
        if (idx < 0) {
            _rowLimitCombo->addItem(QString::number(_displayDefinition.LogViewLimit), _displayDefinition.LogViewLimit);
            idx = _rowLimitCombo->findData(_displayDefinition.LogViewLimit);
        }
        if (idx >= 0)
            _rowLimitCombo->setCurrentIndex(idx);
    }
    if (_exceptionsFilter) {
        QSignalBlocker b(_exceptionsFilter);
        _exceptionsFilter->setChecked(_displayDefinition.ExceptionsOnly);
    }

    trimTrafficBufferToLimit();
    ui->outputWidget->setup(_displayDefinition);
    rebuildVisibleTraffic();
}

///
/// \brief FormTrafficView::backgroundColor
/// \return
///
QColor FormTrafficView::backgroundColor() const
{
    return ui->outputWidget->backgroundColor();
}

///
/// \brief FormTrafficView::setBackgroundColor
/// \param clr
///
void FormTrafficView::setBackgroundColor(const QColor& clr)
{
    ui->outputWidget->setBackgroundColor(clr);
}

///
/// \brief FormTrafficView::foregroundColor
/// \return
///
QColor FormTrafficView::foregroundColor() const
{
    return ui->outputWidget->foregroundColor();
}

///
/// \brief FormTrafficView::setForegroundColor
/// \param clr
///
void FormTrafficView::setForegroundColor(const QColor& clr)
{
    ui->outputWidget->setForegroundColor(clr);
}

///
/// \brief FormTrafficView::font
/// \return
///
QFont FormTrafficView::font() const
{
   return ui->outputWidget->font();
}

///
/// \brief FormTrafficView::setFont
/// \param font
///
void FormTrafficView::setFont(const QFont& font)
{
    ui->outputWidget->setFont(font);
}

///
/// \brief FormTrafficView::logViewState
/// \return
///
LogViewState FormTrafficView::logViewState() const
{
    return _logViewState;
}

///
/// \brief FormTrafficView::setLogViewState
/// \param state
///
void FormTrafficView::setLogViewState(LogViewState state)
{
    if(_logViewState == state)
        return;

    _logViewState = state;
    ui->outputWidget->setLogViewState(state);
    ui->actionPauseTraffic->blockSignals(true);
    ui->actionPauseTraffic->setChecked(state == LogViewState::Paused);
    ui->actionPauseTraffic->blockSignals(false);
    ui->actionPauseTraffic->setEnabled(state != LogViewState::Unknown);
    emit logViewStateChanged(state);
}

///
/// \brief FormTrafficView::setDisplayDefinitionSilent
/// Updates filter controls and definition without emitting definitionChanged.
/// Used for peer sync to prevent signal loops.
///
void FormTrafficView::setDisplayDefinitionSilent(const TrafficViewDefinitions& dd)
{
    _displayDefinition.UnitFilter = dd.UnitFilter;
    _displayDefinition.FunctionCodeFilter = dd.FunctionCodeFilter;
    _displayDefinition.LogViewLimit = dd.LogViewLimit;
    _displayDefinition.ExceptionsOnly = dd.ExceptionsOnly;

    if(_unitIdFilter) {
        QSignalBlocker b(_unitIdFilter);
        _unitIdFilter->setValue(dd.UnitFilter);
    }
    if(_funcCodeFilter) {
        QSignalBlocker b(_funcCodeFilter);
        const int idx = _funcCodeFilter->findData(dd.FunctionCodeFilter);
        _funcCodeFilter->setCurrentIndex(idx < 0 ? 0 : idx);
    }
    if(_rowLimitCombo) {
        QSignalBlocker b(_rowLimitCombo);
        const int idx = _rowLimitCombo->findData(static_cast<int>(dd.LogViewLimit));
        if(idx >= 0) _rowLimitCombo->setCurrentIndex(idx);
    }
    if(_exceptionsFilter) {
        QSignalBlocker b(_exceptionsFilter);
        _exceptionsFilter->setChecked(dd.ExceptionsOnly);
    }

    trimTrafficBufferToLimit();
    ui->outputWidget->setLogViewLimit(dd.LogViewLimit);
    rebuildVisibleTraffic();
}

///
/// \brief FormTrafficView::linkTo
/// Bidirectionally syncs filter settings and pause state with \a other.
///
void FormTrafficView::linkTo(FormTrafficView* other)
{
    if(!other) return;
    connect(this,  &FormTrafficView::definitionChanged, other, [this, other]() {
        other->setDisplayDefinitionSilent(displayDefinition());
    });
    connect(other, &FormTrafficView::definitionChanged, this, [this, other]() {
        setDisplayDefinitionSilent(other->displayDefinition());
    });
    connect(this,  &FormTrafficView::logViewStateChanged, other, &FormTrafficView::setLogViewState);
    connect(other, &FormTrafficView::logViewStateChanged, this,  &FormTrafficView::setLogViewState);
}

///
/// \brief FormTrafficView::show
///
void FormTrafficView::show()
{
    QWidget::show();
    connectEditSlots();

    emit showed();
}

///
/// \brief FormTrafficView::connectEditSlots
///
void FormTrafficView::connectEditSlots()
{
}

///
/// \brief FormTrafficView::disconnectEditSlots
///
void FormTrafficView::disconnectEditSlots()
{
}

///
/// \brief FormTrafficView::on_mbConnected
///
void FormTrafficView::on_mbConnected(const ConnectionDetails&)
{
    updateSourceFilter();
    clearTrafficLog();

    if(logViewState() == LogViewState::Unknown) {
        setLogViewState(LogViewState::Running);
    }
}

///
/// \brief FormTrafficView::on_mbDisconnected
///
void FormTrafficView::on_mbDisconnected(const ConnectionDetails&)
{
    updateSourceFilter();
    if(!_mbMultiServer.isConnected()) {
        setLogViewState(LogViewState::Unknown);
    }
}

///
/// \brief FormTrafficView::matchesTrafficFilter
/// \param msg
/// \return
///
QString FormTrafficView::sourceFilterText(const ConnectionDetails& cd) const
{
    if (cd.Type == ConnectionType::Tcp)
        return tr("Modbus/TCP Srv %1:%2").arg(cd.TcpParams.IPAddress, QString::number(cd.TcpParams.ServicePort));

    return tr("Port %1:%2:%3:%4:%5").arg(cd.SerialParams.PortName,
                                          QString::number(cd.SerialParams.BaudRate),
                                          QString::number(cd.SerialParams.WordLength),
                                          Parity_toString(cd.SerialParams.Parity),
                                          QString::number(cd.SerialParams.StopBits));
}

///
/// \brief FormTrafficView::updateSourceFilter
///
void FormTrafficView::updateSourceFilter()
{
    if (!_sourceFilter)
        return;

    const QVariant selectedData = _sourceFilter->currentData();
    {
        QSignalBlocker b(_sourceFilter);
        _sourceFilter->clear();
        _sourceFilter->addItem(tr("All"), QVariant());
        const auto activeConnections = _mbMultiServer.connections();
        for (const auto& cd : activeConnections)
            _sourceFilter->addItem(sourceFilterText(cd), QVariant::fromValue(cd));
    }

    int idx = _sourceFilter->findData(selectedData);
    if (idx < 0)
        idx = 0;
    _sourceFilter->setCurrentIndex(idx);
}

///
/// \brief FormTrafficView::matchesTrafficFilter
///
bool FormTrafficView::matchesTrafficFilter(const ConnectionDetails& cd,
                                            QSharedPointer<const ModbusMessage> filterMsg,
                                            QSharedPointer<const ModbusMessage> displayMsg) const
{
    if (!filterMsg)
        return false;

    if (!_unitIdFilter || !_funcCodeFilter)
        return true;

    const int unitId = _unitIdFilter->value();
    if (unitId != 0 && filterMsg->deviceId() != unitId)
        return false;

    if (_funcCodeFilter->currentIndex() > 0) {
        const int fc = _funcCodeFilter->currentData().toInt();
        if (static_cast<int>(filterMsg->functionCode()) != fc)
            return false;
    }

    if (_sourceFilter && _sourceFilter->currentIndex() > 0) {
        const auto selected = _sourceFilter->currentData().value<ConnectionDetails>();
        if (!(selected == cd))
            return false;
    }

    if (_exceptionsFilter && _exceptionsFilter->isChecked()) {
        const auto& msg = displayMsg ? displayMsg : filterMsg;
        if (!msg->isException())
            return false;
    }

    return true;
}

///
/// \brief FormTrafficView::on_mbRequest
/// \param msg
///
void FormTrafficView::on_mbRequest(const ConnectionDetails& cd, QSharedPointer<const ModbusMessage> msg)
{
    appendTrafficEntry(cd, msg, msg, true);
}

///
/// \brief FormTrafficView::on_mbResponse
/// \param msgReq
/// \param msgResp
///
void FormTrafficView::on_mbResponse(const ConnectionDetails& cd, QSharedPointer<const ModbusMessage> msgReq, QSharedPointer<const ModbusMessage> msgResp)
{
    appendTrafficEntry(cd, msgResp, msgReq ? msgReq : msgResp, false);
}

///
/// \brief FormTrafficView::on_mbDataChanged
///
void FormTrafficView::on_mbDataChanged(quint8 deviceId, const QModbusDataUnit&)
{
    Q_UNUSED(deviceId);
}

///
/// \brief FormTrafficView::on_mbDefinitionsChanged
/// \param defs
///
void FormTrafficView::on_mbDefinitionsChanged(const ModbusDefinitions& defs)
{
    Q_UNUSED(defs);
}

///
/// \brief FormTrafficView::setupToolbarActions
///
void FormTrafficView::setupToolbarActions()
{
    ui->toolBarTraffic->setVisible(true);
    ui->actionPauseTraffic->setChecked(false);

    connect(ui->actionPauseTraffic, &QAction::toggled, this, [this](bool paused) {
        if (_logViewState == LogViewState::Unknown)
            return;
        setLogViewState(paused ? LogViewState::Paused : LogViewState::Running);
    });

    connect(ui->actionClearTraffic, &QAction::triggered, this, [this]() {
        clearTrafficLog();
    });

    connect(ui->actionExportTrafficLog, &QAction::triggered, this, [this]() {
        const auto filename = QFileDialog::getSaveFileName(this, QString(), QString(), tr("Text files (*.txt)"));
        if (filename.isEmpty())
            return;

        if (ui->outputWidget->exportLogToTextFile(filename))
            QMessageBox::information(this, windowTitle(), tr("Log exported successfully to file %1").arg(filename));
        else
            QMessageBox::critical(this, windowTitle(), tr("Export log error!"));
    });

    ui->toolBarTraffic->removeAction(ui->actionPauseTraffic);
    ui->toolBarTraffic->removeAction(ui->actionClearTraffic);
    ui->toolBarTraffic->removeAction(ui->actionExportTrafficLog);
}

///
/// \brief FormTrafficView::setupFilterControls
///
void FormTrafficView::setupFilterControls()
{
    _labelUnitId = new QLabel(ui->toolBarTraffic);
    _labelUnitId->setText(tr("Unit:"));

    _unitIdFilter = new QSpinBox(ui->toolBarTraffic);
    _unitIdFilter->setRange(0, 247);
    _unitIdFilter->setValue(0);
    _unitIdFilter->setSpecialValueText(tr("All"));
    _unitIdFilter->setToolTip(tr("0 = all unit ids"));
    connect(_unitIdFilter, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        if (_unitIdFilter)
            _displayDefinition.UnitFilter = static_cast<quint8>(_unitIdFilter->value());
        rebuildVisibleTraffic();
        emit definitionChanged();
    });

    _labelFuncCode = new QLabel(ui->toolBarTraffic);
    _labelFuncCode->setText(tr("Function:"));

    _funcCodeFilter = new QComboBox(ui->toolBarTraffic);
    _funcCodeFilter->addItem(tr("All"), -1);
    _funcCodeFilter->addItem(QStringLiteral("01 Read Coils"), static_cast<int>(QModbusPdu::ReadCoils));
    _funcCodeFilter->addItem(QStringLiteral("02 Read Discrete Inputs"), static_cast<int>(QModbusPdu::ReadDiscreteInputs));
    _funcCodeFilter->addItem(QStringLiteral("03 Read Holding Registers"), static_cast<int>(QModbusPdu::ReadHoldingRegisters));
    _funcCodeFilter->addItem(QStringLiteral("04 Read Input Registers"), static_cast<int>(QModbusPdu::ReadInputRegisters));
    _funcCodeFilter->addItem(QStringLiteral("05 Write Single Coil"), static_cast<int>(QModbusPdu::WriteSingleCoil));
    _funcCodeFilter->addItem(QStringLiteral("06 Write Single Register"), static_cast<int>(QModbusPdu::WriteSingleRegister));
    _funcCodeFilter->addItem(QStringLiteral("15 Write Multiple Coils"), static_cast<int>(QModbusPdu::WriteMultipleCoils));
    _funcCodeFilter->addItem(QStringLiteral("16 Write Multiple Registers"), static_cast<int>(QModbusPdu::WriteMultipleRegisters));
    _funcCodeFilter->addItem(QStringLiteral("22 Mask Write Register"), static_cast<int>(QModbusPdu::MaskWriteRegister));
    _funcCodeFilter->addItem(QStringLiteral("23 Read/Write Multiple Registers"), static_cast<int>(QModbusPdu::ReadWriteMultipleRegisters));
    connect(_funcCodeFilter, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        if (_funcCodeFilter)
            _displayDefinition.FunctionCodeFilter = static_cast<qint16>(_funcCodeFilter->currentData().toInt());
        rebuildVisibleTraffic();
        emit definitionChanged();
    });

    _labelSource = new QLabel(ui->toolBarTraffic);
    _labelSource->setText(tr("Source:"));

    _sourceFilter = new QComboBox(ui->toolBarTraffic);
    _sourceFilter->addItem(tr("All"), QVariant());
    _sourceFilter->setMinimumWidth(220);
    _sourceFilter->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(_sourceFilter, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        rebuildVisibleTraffic();
    });

    _exceptionsFilter = new QCheckBox(tr("Show Exceptions"), ui->toolBarTraffic);
    _exceptionsFilter->setToolTip(tr("Show only responses with Modbus exception"));
    connect(_exceptionsFilter, &QCheckBox::toggled, this, [this](bool checked) {
        _displayDefinition.ExceptionsOnly = checked;
        rebuildVisibleTraffic();
        emit definitionChanged();
    });

    _labelRowLimit = new QLabel(ui->toolBarTraffic);
    _labelRowLimit->setText(tr("Rows:"));

    _rowLimitCombo = new QComboBox(ui->toolBarTraffic);
    _rowLimitCombo->setEditable(false);
    _rowLimitCombo->addItem(QStringLiteral("30"), 30);
    _rowLimitCombo->addItem(QStringLiteral("100"), 100);
    _rowLimitCombo->addItem(QStringLiteral("300"), 300);
    _rowLimitCombo->addItem(QStringLiteral("1000"), 1000);
    connect(_rowLimitCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (idx < 0)
            return;
        const int limit = _rowLimitCombo->itemData(idx).toInt();
        if (limit <= 0)
            return;
        _displayDefinition.LogViewLimit = static_cast<quint16>(limit);
        trimTrafficBufferToLimit();
        ui->outputWidget->setLogViewLimit(limit);
        rebuildVisibleTraffic();
        emit definitionChanged();
    });
}

///
/// \brief FormTrafficView::setupToolbarLayout
///
void FormTrafficView::setupToolbarLayout()
{
    ui->toolBarTraffic->addWidget(_labelUnitId);
    addToolbarSpacer(3);
    ui->toolBarTraffic->addWidget(_unitIdFilter);
    addToolbarSpacer(8);

    ui->toolBarTraffic->addWidget(_labelFuncCode);
    addToolbarSpacer(3);
    ui->toolBarTraffic->addWidget(_funcCodeFilter);
    addToolbarSpacer(8);

    ui->toolBarTraffic->addWidget(_labelSource);
    addToolbarSpacer(3);
    ui->toolBarTraffic->addWidget(_sourceFilter);
    addToolbarSpacer(8);

    ui->toolBarTraffic->addWidget(_exceptionsFilter);
    addToolbarSpacer(8);

    ui->toolBarTraffic->addWidget(_labelRowLimit);
    addToolbarSpacer(3);
    ui->toolBarTraffic->addWidget(_rowLimitCombo);

    _trafficFilterStretch = new QWidget(ui->toolBarTraffic);
    _trafficFilterStretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->toolBarTraffic->addWidget(_trafficFilterStretch);
    ui->toolBarTraffic->addAction(ui->actionPauseTraffic);
    ui->toolBarTraffic->addSeparator();
    ui->toolBarTraffic->addAction(ui->actionClearTraffic);
    ui->toolBarTraffic->addSeparator();
    ui->toolBarTraffic->addAction(ui->actionExportTrafficLog);
}

///
/// \brief FormTrafficView::initializeDisplayDefinition
///
void FormTrafficView::initializeDisplayDefinition()
{
    _displayDefinition.FormName = windowTitle();
    _displayDefinition.UnitFilter = 0;
    _displayDefinition.FunctionCodeFilter = -1;
    _displayDefinition.LogViewLimit = ui->outputWidget->logViewLimit();
    _displayDefinition.normalize();

    if (_unitIdFilter)
        _unitIdFilter->setValue(_displayDefinition.UnitFilter);

    if (_funcCodeFilter) {
        int idx = _funcCodeFilter->findData(_displayDefinition.FunctionCodeFilter);
        if (idx < 0)
            idx = 0;
        _funcCodeFilter->setCurrentIndex(idx);
    }

    if (_rowLimitCombo) {
        int idx = _rowLimitCombo->findData(_displayDefinition.LogViewLimit);
        if (idx < 0) {
            _rowLimitCombo->addItem(QString::number(_displayDefinition.LogViewLimit), _displayDefinition.LogViewLimit);
            idx = _rowLimitCombo->findData(_displayDefinition.LogViewLimit);
        }
        if (idx >= 0)
            _rowLimitCombo->setCurrentIndex(idx);
    }

    if (_exceptionsFilter)
        _exceptionsFilter->setChecked(_displayDefinition.ExceptionsOnly);
}

///
/// \brief FormTrafficView::setupServerConnections
///
void FormTrafficView::setupServerConnections()
{
    connect(&_mbMultiServer, &ModbusMultiServer::requestOnConnection, this, &FormTrafficView::on_mbRequest);
    connect(&_mbMultiServer, &ModbusMultiServer::responseOnConnection, this, &FormTrafficView::on_mbResponse);
    connect(&_mbMultiServer, &ModbusMultiServer::connected, this, &FormTrafficView::on_mbConnected);
    connect(&_mbMultiServer, &ModbusMultiServer::disconnected, this, &FormTrafficView::on_mbDisconnected);
    connect(&_mbMultiServer, &ModbusMultiServer::definitionsChanged, this, &FormTrafficView::on_mbDefinitionsChanged);
}

///
/// \brief FormTrafficView::addToolbarSpacer
/// \param width
///
void FormTrafficView::addToolbarSpacer(int width)
{
    auto* spacer = new QWidget(ui->toolBarTraffic);
    spacer->setFixedWidth(width);
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    ui->toolBarTraffic->addWidget(spacer);
}

///
/// \brief FormTrafficView::resetTrafficCounters
///
void FormTrafficView::resetTrafficCounters()
{
    _requestCount = 0;
    _responseCount = 0;
}

///
/// \brief FormTrafficView::clearTrafficLog
///
void FormTrafficView::clearTrafficLog()
{
    _trafficBuffer.clear();
    ui->outputWidget->clearLogView();
    resetTrafficCounters();
}

///
/// \brief FormTrafficView::trimTrafficBufferToLimit
///
void FormTrafficView::trimTrafficBufferToLimit()
{
    const int limit = qMax(1, static_cast<int>(_displayDefinition.LogViewLimit));
    while(_trafficBuffer.size() > limit)
        _trafficBuffer.dequeue();
}

///
/// \brief FormTrafficView::rebuildVisibleTraffic
///
void FormTrafficView::rebuildVisibleTraffic()
{
    const bool paused = (_logViewState == LogViewState::Paused);
    if(paused)
        ui->outputWidget->setLogViewState(LogViewState::Running);

    ui->outputWidget->clearLogView();
    resetTrafficCounters();

    for(const auto& entry : _trafficBuffer)
    {
        if(!entry.DisplayMessage)
            continue;

        if(matchesTrafficFilter(entry.Connection, entry.FilterMessage, entry.DisplayMessage))
        {
            if(entry.IsRequest)
                ++_requestCount;
            else
                ++_responseCount;

            ui->outputWidget->updateTraffic(entry.DisplayMessage);
        }
    }

    if(paused)
        ui->outputWidget->setLogViewState(LogViewState::Paused);
}

///
/// \brief FormTrafficView::appendTrafficEntry
///
void FormTrafficView::appendTrafficEntry(const ConnectionDetails& cd,
                                         const QSharedPointer<const ModbusMessage>& displayMessage,
                                         const QSharedPointer<const ModbusMessage>& filterMessage,
                                         bool isRequest)
{
    if(!displayMessage)
        return;

    TrafficLogEntry entry;
    entry.Connection = cd;
    entry.DisplayMessage = displayMessage;
    entry.FilterMessage = filterMessage ? filterMessage : displayMessage;
    entry.IsRequest = isRequest;
    _trafficBuffer.enqueue(entry);
    trimTrafficBufferToLimit();

    if(matchesTrafficFilter(cd, entry.FilterMessage, entry.DisplayMessage))
    {
        if(isRequest)
            ++_requestCount;
        else
            ++_responseCount;

        ui->outputWidget->updateTraffic(displayMessage);
    }
}







