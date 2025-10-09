#include <QPainter>
#include <QPalette>
#include <QDateTime>
#include <QHelpEngine>
#include <QHelpContentWidget>
#include "modbuslimits.h"
#include "mainwindow.h"
#include "modbusmessages.h"
#include "datasimulator.h"
#include "dialogwritecoilregister.h"
#include "dialogwriteholdingregister.h"
#include "dialogwriteholdingregisterbits.h"
#include "formmodsim.h"
#include "ui_formmodsim.h"

QVersionNumber FormModSim::VERSION = QVersionNumber(1, 11);

///
/// \brief FormModSim::FormModSim
/// \param num
/// \param parent
///
FormModSim::FormModSim(int id, ModbusMultiServer& server, QSharedPointer<DataSimulator> simulator, MainWindow* parent)
    : QWidget(parent)
    , ui(new Ui::FormModSim)
    ,_parent(parent)
    ,_formId(id)
    ,_mbMultiServer(server)
    ,_dataSimulator(simulator)
    ,_verboseLogging(true)
{
    Q_ASSERT(parent != nullptr);

    ui->setupUi(this);
    setWindowTitle(QString("ModSim%1").arg(_formId));

    ui->lineEditDeviceId->setInputRange(ModbusLimits::slaveRange());
    ui->lineEditDeviceId->setValue(1);
    server.addDeviceId(ui->lineEditDeviceId->value<int>());

    ui->stackedWidget->setCurrentIndex(0);
    ui->scriptControl->setModbusMultiServer(&_mbMultiServer);
    ui->scriptControl->setByteOrder(ui->outputWidget->byteOrder());

    ui->lineEditAddress->setPaddingZeroes(true);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(true));
    ui->lineEditAddress->setValue(0);

    ui->lineEditLength->setInputRange(ModbusLimits::lengthRange());
    ui->lineEditLength->setValue(100);

    ui->comboBoxAddressBase->setCurrentAddressBase(AddressBase::Base1);
    ui->comboBoxModbusPointType->setCurrentPointType(QModbusDataUnit::HoldingRegisters);

    onDefinitionChanged();
    ui->outputWidget->setFocus();

    setLogViewState(server.isConnected() ? LogViewState::Running : LogViewState::Unknown);
    connect(ui->statisticWidget, &StatisticWidget::ctrsReseted, ui->outputWidget, &OutputWidget::clearLogView);
    connect(ui->statisticWidget, &StatisticWidget::logStateChanged, ui->outputWidget, &OutputWidget::setLogViewState);

    connect(&_mbMultiServer, &ModbusMultiServer::request, this, &FormModSim::on_mbRequest);
    connect(&_mbMultiServer, &ModbusMultiServer::response, this, &FormModSim::on_mbResponse);
    connect(&_mbMultiServer, &ModbusMultiServer::connected, this, &FormModSim::on_mbConnected);
    connect(&_mbMultiServer, &ModbusMultiServer::disconnected, this, &FormModSim::on_mbDisconnected);
    connect(&_mbMultiServer, &ModbusMultiServer::dataChanged, this, &FormModSim::on_mbDataChanged);

    connect(_dataSimulator.get(), &DataSimulator::simulationStarted, this, &FormModSim::on_simulationStarted);
    connect(_dataSimulator.get(), &DataSimulator::simulationStopped, this, &FormModSim::on_simulationStopped);
    connect(_dataSimulator.get(), &DataSimulator::dataSimulated, this, &FormModSim::on_dataSimulated);
}

///
/// \brief FormModSim::~FormModSim
///
FormModSim::~FormModSim()
{
    delete ui;
}

///
/// \brief FormModSim::changeEvent
/// \param e
///
void FormModSim::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        updateStatus();
    }

    QWidget::changeEvent(e);
}

///
/// \brief FormModSim::closeEvent
/// \param event
///
void FormModSim::closeEvent(QCloseEvent *event)
{
    const auto deviceId = ui->lineEditDeviceId->value<quint8>();
    _mbMultiServer.removeDeviceId(deviceId);
    _mbMultiServer.removeUnitMap(formId(), deviceId);

    emit closing();
    QWidget::closeEvent(event);
}

///
/// \brief FormModSim::filename
/// \return
///
QString FormModSim::filename() const
{
    return _filename;
}

///
/// \brief FormModSim::setFilename
/// \param filename
///
void FormModSim::setFilename(const QString& filename)
{
    _filename = filename;
}

///
/// \brief FormModSim::data
/// \return
///
QVector<quint16> FormModSim::data() const
{
    return ui->outputWidget->data();
}

///
/// \brief FormModSim::displayDefinition
/// \return
///
DisplayDefinition FormModSim::displayDefinition() const
{
    DisplayDefinition dd;
    dd.FormName = windowTitle();
    dd.DeviceId = ui->lineEditDeviceId->value<int>();
    dd.PointAddress = ui->lineEditAddress->value<int>();
    dd.PointType = ui->comboBoxModbusPointType->currentPointType();
    dd.Length = ui->lineEditLength->value<int>();
    dd.ZeroBasedAddress = ui->lineEditAddress->range<int>().from() == 0;
    dd.LogViewLimit = ui->outputWidget->logViewLimit();
    dd.AutoscrollLog = ui->outputWidget->autoscrollLogView();
    dd.VerboseLogging = _verboseLogging;
    dd.HexAddress = displayHexAddresses();

    return dd;
}

///
/// \brief FormModSim::setDisplayDefinition
/// \param dd
///
void FormModSim::setDisplayDefinition(const DisplayDefinition& dd)
{
    if(!dd.FormName.isEmpty())
        setWindowTitle(dd.FormName);

    ui->lineEditDeviceId->setValue(dd.DeviceId);

    ui->comboBoxAddressBase->blockSignals(true);
    ui->comboBoxAddressBase->setCurrentAddressBase(dd.ZeroBasedAddress ? AddressBase::Base0 : AddressBase::Base1);
    ui->comboBoxAddressBase->blockSignals(false);

    ui->lineEditAddress->blockSignals(true);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(dd.ZeroBasedAddress));
    ui->lineEditAddress->setValue(dd.PointAddress);
    ui->lineEditAddress->blockSignals(false);

    ui->lineEditLength->blockSignals(true);
    ui->lineEditLength->setValue(dd.Length);
    ui->lineEditLength->blockSignals(false);

    ui->comboBoxModbusPointType->blockSignals(true);
    ui->comboBoxModbusPointType->setCurrentPointType(dd.PointType);
    ui->comboBoxModbusPointType->blockSignals(false);

    ui->outputWidget->setLogViewLimit(dd.LogViewLimit);
    ui->outputWidget->setAutosctollLogView(dd.AutoscrollLog);
    _verboseLogging = dd.VerboseLogging;

    setDisplayHexAddresses(dd.HexAddress);

    onDefinitionChanged();
}

///
/// \brief FormModSim::displayMode
/// \return
///
DisplayMode FormModSim::displayMode() const
{
    if(ui->stackedWidget->currentIndex() == 1)
        return DisplayMode::Script;
    else
        return ui->outputWidget->displayMode();
}

///
/// \brief FormModSim::setDisplayMode
/// \param mode
///
void FormModSim::setDisplayMode(DisplayMode mode)
{
    switch(mode)
    {
        case DisplayMode::Script:
            ui->stackedWidget->setCurrentIndex(1);
            ui->scriptControl->setFocus();
        break;

        default:
            ui->stackedWidget->setCurrentIndex(0);
            ui->outputWidget->setDisplayMode(mode);
        break;
    }

    emit displayModeChanged(mode);
}

///
/// \brief FormModSim::dataDisplayMode
/// \return
///
DataDisplayMode FormModSim::dataDisplayMode() const
{
    return ui->outputWidget->dataDisplayMode();
}

///
/// \brief FormModSim::displayHexAddresses
/// \return
///
bool FormModSim::displayHexAddresses() const
{
    return ui->outputWidget->displayHexAddresses();
}

///
/// \brief FormModSim::setDisplayHexAddresses
/// \param on
///
void FormModSim::setDisplayHexAddresses(bool on)
{
    ui->outputWidget->setDisplayHexAddresses(on);

    ui->lineEditAddress->setInputMode(on ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(ui->comboBoxAddressBase->currentAddressBase() == AddressBase::Base0));
}

///
/// \brief FormModSim::captureMode
///
CaptureMode FormModSim::captureMode() const
{
    return ui->outputWidget->captureMode();
}

///
/// \brief FormModSim::startTextCapture
/// \param file
///
void FormModSim::startTextCapture(const QString& file)
{
    ui->outputWidget->startTextCapture(file);
}

///
/// \brief FormModSim::stopTextCapture
///
void FormModSim::stopTextCapture()
{
    ui->outputWidget->stopTextCapture();
}

///
/// \brief FormModSim::setDataDisplayMode
/// \param mode
///
void FormModSim::setDataDisplayMode(DataDisplayMode mode)
{
    ui->outputWidget->setDataDisplayMode(mode);
}

///
/// \brief FormModSim::scriptSettings
/// \return
///
ScriptSettings FormModSim::scriptSettings() const
{
    return _scriptSettings;
}

///
/// \brief FormModSim::setScriptSettings
/// \param ss
///
void FormModSim::setScriptSettings(const ScriptSettings& ss)
{
    _scriptSettings = ss;
    ui->scriptControl->enableAutoComplete(ss.UseAutoComplete);
    emit scriptSettingsChanged(ss);
}

///
/// \brief FormModSim::byteOrder
/// \return
///
ByteOrder FormModSim::byteOrder() const
{
    return *ui->outputWidget->byteOrder();
}

///
/// \brief FormModSim::setByteOrder
/// \param order
///
void FormModSim::setByteOrder(ByteOrder order)
{
    ui->outputWidget->setByteOrder(order);
    emit byteOrderChanged(order);
}

///
/// \brief FormModSim::codepage
/// \return
///
QString FormModSim::codepage() const
{
    return ui->outputWidget->codepage();
}

///
/// \brief FormModSim::setCodepage
/// \param name
///
void FormModSim::setCodepage(const QString& name)
{
    ui->outputWidget->setCodepage(name);
    emit codepageChanged(name);
}

///
/// \brief FormModSim::backgroundColor
/// \return
///
QColor FormModSim::backgroundColor() const
{
    return ui->outputWidget->backgroundColor();
}

///
/// \brief FormModSim::setBackgroundColor
/// \param clr
///
void FormModSim::setBackgroundColor(const QColor& clr)
{
    ui->outputWidget->setBackgroundColor(clr);
}

///
/// \brief FormModSim::foregroundColor
/// \return
///
QColor FormModSim::foregroundColor() const
{
    return ui->outputWidget->foregroundColor();
}

///
/// \brief FormModSim::setForegroundColor
/// \param clr
///
void FormModSim::setForegroundColor(const QColor& clr)
{
    ui->outputWidget->setForegroundColor(clr);
}

///
/// \brief FormModSim::statusColor
/// \return
///
QColor FormModSim::statusColor() const
{
    return ui->outputWidget->statusColor();
}

///
/// \brief FormModSim::setStatusColor
/// \param clr
///
void FormModSim::setStatusColor(const QColor& clr)
{
    ui->outputWidget->setStatusColor(clr);
}

///
/// \brief FormModSim::font
/// \return
///
QFont FormModSim::font() const
{
   return ui->outputWidget->font();
}

///
/// \brief FormModSim::setFont
/// \param font
///
void FormModSim::setFont(const QFont& font)
{
    ui->outputWidget->setFont(font);
}

///
/// \brief FormModSim::print
/// \param printer
///
void FormModSim::print(QPrinter* printer)
{
    if(!printer) return;

    auto layout = printer->pageLayout();
    const auto resolution = printer->resolution();
    auto pageRect = layout.paintRectPixels(resolution);

    const auto margin = qMax(pageRect.width(), pageRect.height()) * 0.05;
    layout.setMargins(QMargins(margin, margin, margin, margin));
    pageRect.adjust(layout.margins().left(), layout.margins().top(), -layout.margins().right(), -layout.margins().bottom());

    const int pageWidth = pageRect.width();
    const int pageHeight = pageRect.height();

    const int cx = pageRect.left();
    const int cy = pageRect.top();

    QPainter painter(printer);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    const auto textTime = QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat);
    auto rcTime = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextSingleLine, textTime);

    const auto textAddrLen = QString(tr("Address Base: %1\nStart Address: %2\nLength: %3")).arg(ui->comboBoxAddressBase->currentText(), ui->lineEditAddress->text(), ui->lineEditLength->text());
    auto rcAddrLen = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextWordWrap, textAddrLen);

    const auto textDevIdType = QString(tr("Device Id: %1\nMODBUS Point Type:\n%2")).arg(ui->lineEditDeviceId->text(), ui->comboBoxModbusPointType->currentText());
    auto rcDevIdType = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextWordWrap, textDevIdType);

    rcTime.moveTopRight({ pageRect.right(), 10 });
    rcDevIdType.moveTopLeft({ rcAddrLen.right() + 40, rcAddrLen.top()});

    painter.drawText(rcTime, Qt::TextSingleLine, textTime);
    painter.drawText(rcAddrLen, Qt::TextWordWrap, textAddrLen);
    painter.drawText(rcDevIdType, Qt::TextWordWrap, textDevIdType);

    auto rcOutput = pageRect;
    rcOutput.setTop(rcAddrLen.bottom() + 20);

    ui->outputWidget->paint(rcOutput, painter);
}

///
/// \brief FormModSim::simulationMap
/// \return
///
ModbusSimulationMap2 FormModSim::simulationMap() const
{
    const auto dd = displayDefinition();
    const auto startAddr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    const auto endAddr = startAddr + dd.Length;

    ModbusSimulationMap2 result;
    const auto simulationMap = _dataSimulator->simulationMap();
    for(auto&& key : simulationMap.keys())
    {
        if(key.DeviceId == dd.DeviceId &&
           key.Type == dd.PointType &&
           key.Address >= startAddr && key.Address < endAddr)
        {
            result[key] = simulationMap[key];
        }
    }

    return result;
}

///
/// \brief FormModSim::serializeModbusDataUnit
/// \param deviceId
/// \param type
/// \param startAddress
/// \param length
/// \return
///
QModbusDataUnit FormModSim::serializeModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, quint16 length) const
{
    QModbusDataUnit dataUnit;
    const auto serverData = _mbMultiServer.data(deviceId, type, startAddress, length);

    if (startAddress >= serverData.startAddress() &&
        (startAddress + length) <= (serverData.startAddress() + serverData.valueCount())) {

        const int offset = startAddress - serverData.startAddress();

        QVector<quint16> values;
        for (int i = 0; i < length; ++i) {
            values.append(serverData.value(offset + i));
        }

        dataUnit.setValues(values);
        dataUnit.setRegisterType(type);
        dataUnit.setStartAddress(startAddress);
    }

    return dataUnit;
}

///
/// \brief FormModSim::startSimulation
/// \param type
/// \param addr
/// \param params
///
void FormModSim::startSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params)
{
    const auto deviceId = ui->lineEditDeviceId->value<quint8>();
    _dataSimulator->startSimulation(dataDisplayMode(), deviceId, type, addr, params);
}

///
/// \brief FormModSim::configureModbusDataUnit
/// \param deviceId
/// \param type
/// \param startAddress
/// \param values
///
void FormModSim::configureModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, const QVector<quint16>& values) const
{
    QModbusDataUnit unit;
    unit.setRegisterType(type);
    unit.setStartAddress(startAddress);
    unit.setValues(values);
    _mbMultiServer.setData(deviceId, unit);
}


///
/// \brief FormModSim::descriptionMap
/// \return
///
AddressDescriptionMap2 FormModSim::descriptionMap() const
{
    return ui->outputWidget->descriptionMap();
}

///
/// \brief FormModSim::setDescription
/// \param deviceId
/// \param type
/// \param addr
/// \param desc
///
void FormModSim::setDescription(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc)
{
    ui->outputWidget->setDescription(deviceId, type, addr, desc);
}

///
/// \brief FormModSim::script
/// \return
///
QString FormModSim::script() const
{
    return ui->scriptControl->script();
}

///
/// \brief FormModSim::setScript
/// \param text
///
void FormModSim::setScript(const QString& text)
{
    ui->scriptControl->setScript(text);
}

///
/// \brief FormModSim::searchText
/// \return
///
QString FormModSim::searchText() const
{
    return ui->scriptControl->searchText();
}

///
/// \brief FormModSim::canRunScript
/// \return
///
bool FormModSim::canRunScript() const
{
    return !ui->scriptControl->script().isEmpty() &&
           !ui->scriptControl->isRunning();
}

///
/// \brief FormModSim::canStopScript
/// \return
///
bool FormModSim::canStopScript() const
{
    return ui->scriptControl->isRunning();
}

///
/// \brief FormModSim::canUndo
/// \return
///
bool FormModSim::canUndo() const
{
    return ui->scriptControl->canUndo();
}

///
/// \brief FormModSim::canRedo
/// \return
///
bool FormModSim::canRedo() const
{
    return ui->scriptControl->canRedo();
}

///
/// \brief FormModSim::canPaste
/// \return
///
bool FormModSim::canPaste() const
{
    return ui->scriptControl->canPaste();
}

///
/// \brief FormModSim::runScript
/// \param interval
///
void FormModSim::runScript()
{
    ui->scriptControl->runScript(_scriptSettings.Mode, _scriptSettings.Interval);
}

///
/// \brief FormModSim::stopScript
///
void FormModSim::stopScript()
{
    ui->scriptControl->stopScript();
}

///
/// \brief FormModSim::logViewState
/// \return
///
LogViewState FormModSim::logViewState() const
{
    return ui->statisticWidget->logState();
}

///
/// \brief FormModSim::setLogViewState
/// \param state
///
void FormModSim::setLogViewState(LogViewState state)
{
    ui->statisticWidget->setLogState(state);
}

///
/// \brief FormModSim::show
///
void FormModSim::show()
{
    QWidget::show();
    connectEditSlots();

    emit showed();
}

///
/// \brief FormModSim::on_lineEditAddress_valueChanged
///
void FormModSim::on_lineEditAddress_valueChanged(const QVariant&)
{
   onDefinitionChanged();
}

///
/// \brief FormModSim::on_lineEditLength_valueChanged
///
void FormModSim::on_lineEditLength_valueChanged(const QVariant&)
{
    onDefinitionChanged();
}

///
/// \brief FormModSim::on_lineEditDeviceId_valueChanged
///
void FormModSim::on_lineEditDeviceId_valueChanged(const QVariant& oldValue, const QVariant& newValue)
{
    _mbMultiServer.removeDeviceId(oldValue.toInt());
    _mbMultiServer.addDeviceId(newValue.toInt());

    onDefinitionChanged();
}

///
/// \brief FormModSim::on_comboBoxAddressBase_addressBaseChanged
/// \param base
///
void FormModSim::on_comboBoxAddressBase_addressBaseChanged(AddressBase base)
{
    auto dd = displayDefinition();
    dd.PointAddress = (base == AddressBase::Base1 ? qMax(1, dd.PointAddress + 1) : qMax(0, dd.PointAddress - 1));
    dd.ZeroBasedAddress = (base == AddressBase::Base0);
    setDisplayDefinition(dd);
}

///
/// \brief FormModSim::on_comboBoxModbusPointType_pointTypeChanged
///
void FormModSim::on_comboBoxModbusPointType_pointTypeChanged(QModbusDataUnit::RegisterType type)
{
    onDefinitionChanged();
    emit pointTypeChanged(type);
}

///
/// \brief FormModSim::updateStatus
///
void FormModSim::updateStatus()
{
    const auto dd = displayDefinition();
    if(_mbMultiServer.isConnected())
    {
        const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
        if(addr + dd.Length <= ModbusLimits::addressRange().to())
            ui->outputWidget->setStatus(QString());
        else
            ui->outputWidget->setInvalidLengthStatus();
    }
    else
    {
        ui->outputWidget->setNotConnectedStatus();
    }
}

///
/// \brief FormModSim::onDefinitionChanged
///
void FormModSim::onDefinitionChanged()
{
    updateStatus();

    const auto dd = displayDefinition();
    const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    _mbMultiServer.addUnitMap(formId(), dd.DeviceId, dd.PointType, addr, dd.Length);

    ui->scriptControl->setAddressBase(dd.ZeroBasedAddress ? AddressBase::Base0 : AddressBase::Base1);
    ui->outputWidget->setup(dd, _dataSimulator->simulationMap(), _mbMultiServer.data(dd.DeviceId, dd.PointType, addr, dd.Length));
}

///
/// \brief FormModSim::scriptControl
/// \return
///
ScriptControl* FormModSim::scriptControl()
{
    return ui->scriptControl;
}

///
/// \brief FormModSim::on_outputWidget_itemDoubleClicked
/// \param addr
/// \param value
///
void FormModSim::on_outputWidget_itemDoubleClicked(quint16 addr, const QVariant& value)
{
    const auto mode = dataDisplayMode();
    const auto deviceId = ui->lineEditDeviceId->value<quint8>();
    const auto pointType = ui->comboBoxModbusPointType->currentPointType();
    const auto zeroBasedAddress = displayDefinition().ZeroBasedAddress;
    const auto simAddr = addr - (zeroBasedAddress ? 0 : 1);
    auto simParams = _dataSimulator->simulationParams(deviceId, pointType, addr);

    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
        {
            ModbusWriteParams params = { addr, value, mode, byteOrder(), codepage(), zeroBasedAddress };
            DialogWriteCoilRegister dlg(params, simParams, displayHexAddresses(), this);
            switch(dlg.exec())
            {
                case QDialog::Accepted:
                    _mbMultiServer.writeRegister(deviceId, pointType, params);
                break;

                case 2:
                    if(simParams.Mode == SimulationMode::No) _dataSimulator->stopSimulation(deviceId, pointType, simAddr);
                    else _dataSimulator->startSimulation(mode, deviceId, pointType, simAddr, simParams);
                break;
            }
        }
        break;

        case QModbusDataUnit::InputRegisters:
        case QModbusDataUnit::HoldingRegisters:
        {
            ModbusWriteParams params = { addr, value, mode, byteOrder(), codepage(), zeroBasedAddress };
            if(mode == DataDisplayMode::Binary)
            {
                DialogWriteHoldingRegisterBits dlg(params, displayHexAddresses(), this);
                if(dlg.exec() == QDialog::Accepted)
                    _mbMultiServer.writeRegister(deviceId, pointType, params);
            }
            else
            {
                DialogWriteHoldingRegister dlg(params, simParams, displayHexAddresses(), this);
                switch(dlg.exec())
                {
                    case QDialog::Accepted:
                        _mbMultiServer.writeRegister(deviceId, pointType, params);
                    break;

                    case 2:
                        if(simParams.Mode == SimulationMode::No) _dataSimulator->stopSimulation(deviceId, pointType, simAddr);
                        else _dataSimulator->startSimulation(mode, deviceId, pointType, simAddr, simParams);
                    break;
                }
            }
        }
        break;

        default:
        break;
    }
}

///
/// \brief FormModSim::on_mbConnected
///
void FormModSim::on_mbConnected(const ConnectionDetails&)
{
    updateStatus();
    ui->outputWidget->clearLogView();

    if(logViewState() == LogViewState::Unknown) {
        setLogViewState(LogViewState::Running);
    }
}

///
/// \brief FormModSim::on_mbDisconnected
///
void FormModSim::on_mbDisconnected(const ConnectionDetails&)
{
    updateStatus();
    if(!_mbMultiServer.isConnected()) {
        setLogViewState(LogViewState::Unknown);
    }
}

///
/// \brief FormModSim::isLoggingRequest
/// \param req
/// \param protocol
/// \return
///
bool FormModSim::isLoggingRequest(quint8 deviceId, const QModbusRequest& req, ModbusMessage::ProtocolType protocol) const
{
    const auto dd = displayDefinition();
    if(dd.DeviceId != deviceId)
        return false;

    const auto startAddress = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    auto msg = ModbusMessage::create(req, protocol, dd.DeviceId, QDateTime::currentDateTime(), true);

    switch(req.functionCode()) {
        case QModbusPdu::ReadCoils: {
            auto req = reinterpret_cast<const ReadCoilsRequest*>(msg.get());
            return (req->startAddress() >= startAddress) && (dd.PointType == QModbusDataUnit::Coils);
        }
        case QModbusPdu::ReadDiscreteInputs: {
            auto req = reinterpret_cast<const ReadDiscreteInputsRequest*>(msg.get());
            return (req->startAddress() >= startAddress) && (dd.PointType == QModbusDataUnit::DiscreteInputs);
        }
        case QModbusPdu::ReadHoldingRegisters: {
            auto req = reinterpret_cast<const ReadHoldingRegistersRequest*>(msg.get());
            return (req->startAddress() >= startAddress) && (dd.PointType == QModbusDataUnit::HoldingRegisters);
        }
        case QModbusPdu::ReadInputRegisters: {
            auto req = reinterpret_cast<const ReadInputRegistersRequest*>(msg.get());
            return (req->startAddress() >= startAddress) && (dd.PointType == QModbusDataUnit::InputRegisters);
        }
        case QModbusPdu::WriteSingleCoil: {
            auto req = reinterpret_cast<const WriteSingleCoilRequest*>(msg.get());
            return (req->address() >= startAddress) && (dd.PointType == QModbusDataUnit::Coils);
        }
        case QModbusPdu::WriteSingleRegister: {
            auto req = reinterpret_cast<const WriteSingleRegisterRequest*>(msg.get());
            return (req->address() >= startAddress) && (dd.PointType == QModbusDataUnit::HoldingRegisters);
        }
        case QModbusPdu::WriteMultipleCoils: {
            auto req = reinterpret_cast<const WriteMultipleCoilsRequest*>(msg.get());
            return (req->startAddress() >= startAddress) && (dd.PointType == QModbusDataUnit::Coils);
        }
        case QModbusPdu::WriteMultipleRegisters: {
            auto req = reinterpret_cast<const WriteMultipleRegistersRequest*>(msg.get());
            return (req->startAddress() >= startAddress) && (dd.PointType == QModbusDataUnit::HoldingRegisters);
        }
        case QModbusPdu::MaskWriteRegister: {
            auto req = reinterpret_cast<const MaskWriteRegisterRequest*>(msg.get());
            return (req->address() >= startAddress) && (dd.PointType == QModbusDataUnit::HoldingRegisters);
        }
        case QModbusPdu::ReadWriteMultipleRegisters: {
            auto req = reinterpret_cast<const ReadWriteMultipleRegistersRequest*>(msg.get());
            return ((req->readStartAddress() >= startAddress) || (req->writeStartAddress() >= startAddress)) && (dd.PointType == QModbusDataUnit::HoldingRegisters);
        }
        default:
            return true;
    }
}

///
/// \brief FormModSim::on_mbRequest
/// \param req
/// \param protocol
/// \param transactionId
///
void FormModSim::on_mbRequest(quint8 deviceId, const QModbusRequest& req, ModbusMessage::ProtocolType protocol, int transactionId)
{
    if(_verboseLogging || isLoggingRequest(deviceId, req, protocol)) {
        ui->statisticWidget->increaseRequests();
        ui->outputWidget->updateTraffic(req,  deviceId, transactionId, protocol);
    }
}

///
/// \brief FormModSim::on_mbResponse
/// \param req
/// \param resp
/// \param protocol
/// \param transactionId
///
void FormModSim::on_mbResponse(quint8 deviceId, const QModbusRequest& req, const QModbusResponse& resp, ModbusMessage::ProtocolType protocol, int transactionId)
{
    if(_verboseLogging || isLoggingRequest(deviceId, req, protocol)) {
        ui->statisticWidget->increaseResponses();
        ui->outputWidget->updateTraffic(resp, deviceId, transactionId, protocol);
    }
}

///
/// \brief FormModSim::on_mbDataChanged
///
void FormModSim::on_mbDataChanged(quint8 deviceId, const QModbusDataUnit&)
{
    const auto dd = displayDefinition();
    if(deviceId == dd.DeviceId)
    {
        const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
        ui->outputWidget->updateData(_mbMultiServer.data(deviceId, dd.PointType, addr, dd.Length));
    }
}

///
/// \brief FormModSim::on_simulationStarted
/// \param type
/// \param addr
///
void FormModSim::on_simulationStarted(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr)
{
    if(deviceId != ui->lineEditDeviceId->value<quint8>())
        return;

    ui->outputWidget->setSimulated(deviceId, type, addr, true);
}

///
/// \brief FormModSim::on_simulationStopped
/// \param type
/// \param addr
///
void FormModSim::on_simulationStopped(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr)
{
    if(deviceId != ui->lineEditDeviceId->value<quint8>())
        return;

    ui->outputWidget->setSimulated(deviceId, type, addr, false);
}

///
/// \brief FormModSim::on_dataSimulated
/// \param mode
/// \param type
/// \param addr
/// \param value
///
void FormModSim::on_dataSimulated(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, QVariant value)
{
    const auto dd = displayDefinition();
    const auto pointAddr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    if(deviceId == dd.DeviceId && type == dd.PointType && addr >= pointAddr && addr <= pointAddr + dd.Length)
    {
        _mbMultiServer.writeRegister(dd.DeviceId, type, { addr, value, mode, byteOrder(), codepage(), true });
    }
}

///
/// \brief FormModSim::connectEditSlots
///
void FormModSim::connectEditSlots()
{
    disconnectEditSlots();
    connect(_parent, &MainWindow::undo, ui->scriptControl, &ScriptControl::undo);
    connect(_parent, &MainWindow::redo, ui->scriptControl, &ScriptControl::redo);
    connect(_parent, &MainWindow::cut, ui->scriptControl, &ScriptControl::cut);
    connect(_parent, &MainWindow::copy, ui->scriptControl, &ScriptControl::copy);
    connect(_parent, &MainWindow::paste, ui->scriptControl, &ScriptControl::paste);
    connect(_parent, &MainWindow::selectAll, ui->scriptControl, &ScriptControl::selectAll);
    connect(_parent, &MainWindow::search, ui->scriptControl, &ScriptControl::search);
}

///
/// \brief FormModSim::disconnectEditSlots
///
void FormModSim::disconnectEditSlots()
{
    disconnect(_parent, &MainWindow::undo, ui->scriptControl, &ScriptControl::undo);
    disconnect(_parent, &MainWindow::redo, ui->scriptControl, &ScriptControl::redo);
    disconnect(_parent, &MainWindow::cut, ui->scriptControl, &ScriptControl::cut);
    disconnect(_parent, &MainWindow::copy, ui->scriptControl, &ScriptControl::copy);
    disconnect(_parent, &MainWindow::paste, ui->scriptControl, &ScriptControl::paste);
    disconnect(_parent, &MainWindow::selectAll, ui->scriptControl, &ScriptControl::selectAll);
    disconnect(_parent, &MainWindow::search, ui->scriptControl, &ScriptControl::search);
}
