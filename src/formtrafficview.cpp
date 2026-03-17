#include <QPainter>
#include <QPalette>
#include <QDateTime>
#include <QHelpEngine>
#include <QHelpContentWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include "mainwindow.h"
#include "modbusmessages.h"
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
    ui->toolBarTraffic->setVisible(true);
    ui->actionPauseTraffic->setChecked(false);
    connect(ui->actionPauseTraffic, &QAction::toggled, this, [this](bool paused) {
        if (_logViewState == LogViewState::Unknown)
            return;
        setLogViewState(paused ? LogViewState::Paused : LogViewState::Running);
    });
    connect(ui->actionClearTraffic, &QAction::triggered, this, [this]() {
        ui->outputWidget->clearLogView();
        _requestCount = 0;
        _responseCount = 0;
    });
    ui->toolBarTraffic->addSeparator();

    _labelUnitId = new QLabel(ui->toolBarTraffic);
    _labelUnitId->setText(tr("Unit:"));
    ui->toolBarTraffic->addWidget(_labelUnitId);

    _unitIdFilter = new QSpinBox(ui->toolBarTraffic);
    _unitIdFilter->setRange(0, 247);
    _unitIdFilter->setValue(0);
    _unitIdFilter->setSpecialValueText(tr("All"));
    _unitIdFilter->setToolTip(tr("0 = all unit ids"));
    ui->toolBarTraffic->addWidget(_unitIdFilter);
    connect(_unitIdFilter, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        _requestCount = 0;
        _responseCount = 0;
    });

    _labelFuncCode = new QLabel(ui->toolBarTraffic);
    _labelFuncCode->setText(tr("Function:"));
    ui->toolBarTraffic->addWidget(_labelFuncCode);

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
    ui->toolBarTraffic->addWidget(_funcCodeFilter);
    connect(_funcCodeFilter, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        _requestCount = 0;
        _responseCount = 0;
    });

    _labelRowLimit = new QLabel(ui->toolBarTraffic);
    _labelRowLimit->setText(tr("Rows:"));
    ui->toolBarTraffic->addWidget(_labelRowLimit);

    _rowLimitCombo = new QComboBox(ui->toolBarTraffic);
    _rowLimitCombo->setEditable(false);
    _rowLimitCombo->addItem(QStringLiteral("30"), 30);
    _rowLimitCombo->addItem(QStringLiteral("100"), 100);
    _rowLimitCombo->addItem(QStringLiteral("300"), 300);
    _rowLimitCombo->addItem(QStringLiteral("1000"), 1000);
    ui->toolBarTraffic->addWidget(_rowLimitCombo);
    connect(_rowLimitCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (idx < 0)
            return;
        const int limit = _rowLimitCombo->itemData(idx).toInt();
        if (limit <= 0)
            return;
        ui->outputWidget->setLogViewLimit(limit);
        _displayDefinition.LogViewLimit = static_cast<quint16>(limit);
    });

    const auto mbDefs = server.getModbusDefinitions();
    _displayDefinition.FormName = windowTitle();
    _displayDefinition.DeviceId = 1;
    _displayDefinition.PointAddress = 1;
    _displayDefinition.PointType = QModbusDataUnit::HoldingRegisters;
    _displayDefinition.Length = 100;
    _displayDefinition.LogViewLimit = ui->outputWidget->logViewLimit();
    _displayDefinition.AutoscrollLog = ui->outputWidget->autoscrollLogView();
    _displayDefinition.VerboseLogging = true;
    _displayDefinition.HexAddress = false;
    _displayDefinition.HexViewAddress = false;
    _displayDefinition.HexViewDeviceId = false;
    _displayDefinition.HexViewLength = false;
    _displayDefinition.AddrSpace = mbDefs.AddrSpace;
    _displayDefinition.DataViewColumnsDistance = 16;
    _displayDefinition.LeadingZeros = true;
    _displayDefinition.normalize();
    if (_rowLimitCombo) {
        int idx = _rowLimitCombo->findData(_displayDefinition.LogViewLimit);
        if (idx < 0) {
            _rowLimitCombo->addItem(QString::number(_displayDefinition.LogViewLimit), _displayDefinition.LogViewLimit);
            idx = _rowLimitCombo->findData(_displayDefinition.LogViewLimit);
        }
        if (idx >= 0)
            _rowLimitCombo->setCurrentIndex(idx);
    }

    server.addDeviceId(_displayDefinition.DeviceId);

    ui->outputWidget->setFocus();

    setLogViewState(server.isConnected() ? LogViewState::Running : LogViewState::Unknown);

    connect(&server, &ModbusMultiServer::request, this, &FormTrafficView::on_mbRequest);
    connect(&server, &ModbusMultiServer::response, this, &FormTrafficView::on_mbResponse);
    connect(&server, &ModbusMultiServer::connected, this, &FormTrafficView::on_mbConnected);
    connect(&server, &ModbusMultiServer::disconnected, this, &FormTrafficView::on_mbDisconnected);
    connect(&server, &ModbusMultiServer::definitionsChanged, this, &FormTrafficView::on_mbDefinitionsChanged);

    const auto addr = _displayDefinition.PointAddress - (_displayDefinition.ZeroBasedAddress ? 0 : 1);
    _mbMultiServer.addUnitMap(formId(), _displayDefinition.DeviceId, _displayDefinition.PointType, addr, _displayDefinition.Length);
    ui->outputWidget->setup(_displayDefinition);
}

///
/// \brief FormTrafficView::~FormTrafficView
///
FormTrafficView::~FormTrafficView()
{
    delete ui;
}

void FormTrafficView::saveSettings(QSettings& out) const
{
    out << const_cast<FormTrafficView*>(this);
}

void FormTrafficView::loadSettings(QSettings& in)
{
    in >> this;
}

void FormTrafficView::saveXml(QXmlStreamWriter& xml) const
{
    xml << const_cast<FormTrafficView*>(this);
}

void FormTrafficView::loadXml(QXmlStreamReader& xml)
{
    xml >> this;
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
    const auto deviceId = _displayDefinition.DeviceId;
    _mbMultiServer.removeDeviceId(deviceId);
    _mbMultiServer.removeUnitMap(formId(), deviceId);

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
    dd.LogViewLimit = ui->outputWidget->logViewLimit();
    dd.AutoscrollLog = ui->outputWidget->autoscrollLogView();
    dd.VerboseLogging = _displayDefinition.VerboseLogging;
    dd.HexAddress = _displayDefinition.HexAddress;
    dd.AddrSpace = _mbMultiServer.getModbusDefinitions().AddrSpace;
    dd.DataViewColumnsDistance = _displayDefinition.DataViewColumnsDistance;
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

    const auto defs = _mbMultiServer.getModbusDefinitions();
    next.AddrSpace = defs.AddrSpace;
    next.normalize();

    const auto oldDeviceId = _displayDefinition.DeviceId;
    _displayDefinition = next;

    if (oldDeviceId != _displayDefinition.DeviceId) {
        _mbMultiServer.removeDeviceId(oldDeviceId);
        _mbMultiServer.removeUnitMap(formId(), oldDeviceId);
        _mbMultiServer.addDeviceId(_displayDefinition.DeviceId);
    }

    ui->outputWidget->setLogViewLimit(_displayDefinition.LogViewLimit);
    ui->outputWidget->setAutosctollLogView(_displayDefinition.AutoscrollLog);
    if (_rowLimitCombo) {
        int idx = _rowLimitCombo->findData(_displayDefinition.LogViewLimit);
        if (idx < 0) {
            _rowLimitCombo->addItem(QString::number(_displayDefinition.LogViewLimit), _displayDefinition.LogViewLimit);
            idx = _rowLimitCombo->findData(_displayDefinition.LogViewLimit);
        }
        if (idx >= 0)
            _rowLimitCombo->setCurrentIndex(idx);
    }

    const auto addr = _displayDefinition.PointAddress - (_displayDefinition.ZeroBasedAddress ? 0 : 1);
    _mbMultiServer.addUnitMap(formId(), _displayDefinition.DeviceId, _displayDefinition.PointType, addr, _displayDefinition.Length);
    ui->outputWidget->setup(_displayDefinition);
}

FormDisplayDefinition FormTrafficView::displayDefinitionValue() const
{
    return displayDefinition();
}

void FormTrafficView::setDisplayDefinitionValue(const FormDisplayDefinition& dd)
{
    if (const auto value = std::get_if<TrafficViewDefinitions>(&dd))
        setDisplayDefinition(*value);
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
/// \brief FormTrafficView::on_mbConnected
///
void FormTrafficView::on_mbConnected(const ConnectionDetails&)
{
    ui->outputWidget->clearLogView();

    if(logViewState() == LogViewState::Unknown) {
        setLogViewState(LogViewState::Running);
    }
}

///
/// \brief FormTrafficView::on_mbDisconnected
///
void FormTrafficView::on_mbDisconnected(const ConnectionDetails&)
{
    if(!_mbMultiServer.isConnected()) {
        setLogViewState(LogViewState::Unknown);
    }
}

///
/// \brief FormTrafficView::matchesTrafficFilter
/// \param msg
/// \return
///
bool FormTrafficView::matchesTrafficFilter(QSharedPointer<const ModbusMessage> msg) const
{
    if (!msg)
        return false;

    if (!_unitIdFilter || !_funcCodeFilter)
        return true;

    const int unitId = _unitIdFilter->value();
    if (unitId != 0 && msg->deviceId() != unitId)
        return false;

    if (_funcCodeFilter->currentIndex() > 0) {
        const int fc = _funcCodeFilter->currentData().toInt();
        if (static_cast<int>(msg->functionCode()) != fc)
            return false;
    }

    return true;
}

///
/// \brief FormTrafficView::on_mbRequest
/// \param msg
///
void FormTrafficView::on_mbRequest(QSharedPointer<const ModbusMessage> msg)
{
    if(matchesTrafficFilter(msg)) {
        ++_requestCount;
        ui->outputWidget->updateTraffic(msg);
    }
}

///
/// \brief FormTrafficView::on_mbResponse
/// \param msgReq
/// \param msgResp
///
void FormTrafficView::on_mbResponse(QSharedPointer<const ModbusMessage> msgReq, QSharedPointer<const ModbusMessage> msgResp)
{
    const bool passesFilter = msgReq ? matchesTrafficFilter(msgReq) : matchesTrafficFilter(msgResp);
    if(passesFilter) {
        ++_responseCount;
        ui->outputWidget->updateTraffic(msgResp);
    }
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
    _displayDefinition.AddrSpace = defs.AddrSpace;
    _displayDefinition.normalize();
    const auto dd = displayDefinition();
    const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    ui->outputWidget->setup(dd);
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







