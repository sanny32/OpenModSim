#include <QPainter>
#include <QPalette>
#include <QDateTime>
#include <QHelpEngine>
#include <QHelpContentWidget>
#include "modbuslimits.h"
#include "mainwindow.h"
#include "datasimulator.h"
#include "dialogwritecoilregister.h"
#include "dialogwriteholdingregister.h"
#include "dialogwriteholdingregisterbits.h"
#include "formmodsim.h"
#include "ui_formmodsim.h"

QVersionNumber FormModSim::VERSION = QVersionNumber(1, 6);

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
{
    Q_ASSERT(parent != nullptr);

    ui->setupUi(this);
    setWindowTitle(QString("ModSim%1").arg(_formId));

    ui->lineEditDeviceId->setInputRange(ModbusLimits::slaveRange());
    ui->lineEditDeviceId->setValue(server.deviceId());

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

    connect(&_mbMultiServer, &ModbusMultiServer::request, this, &FormModSim::on_mbRequest);
    connect(&_mbMultiServer, &ModbusMultiServer::response, this, &FormModSim::on_mbResponse);
    connect(&_mbMultiServer, &ModbusMultiServer::connected, this, &FormModSim::on_mbConnected);
    connect(&_mbMultiServer, &ModbusMultiServer::disconnected, this, &FormModSim::on_mbDisconnected);
    connect(&_mbMultiServer, &ModbusMultiServer::deviceIdChanged, this, &FormModSim::on_mbDeviceIdChanged);
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
    _mbMultiServer.removeUnitMap(formId());

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
    dd.DeviceId = ui->lineEditDeviceId->value<int>();
    dd.PointAddress = ui->lineEditAddress->value<int>();
    dd.PointType = ui->comboBoxModbusPointType->currentPointType();
    dd.Length = ui->lineEditLength->value<int>();
    dd.ZeroBasedAddress = ui->lineEditAddress->range<int>().from() == 0;

    return dd;
}

///
/// \brief FormModSim::setDisplayDefinition
/// \param dd
///
void FormModSim::setDisplayDefinition(const DisplayDefinition& dd)
{
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
}

///
/// \brief FormModSca::captureMode
///
CaptureMode FormModSim::captureMode() const
{
    return ui->outputWidget->captureMode();
}

///
/// \brief FormModSca::startTextCapture
/// \param file
///
void FormModSim::startTextCapture(const QString& file)
{
    ui->outputWidget->startTextCapture(file);
}

///
/// \brief FormModSca::stopTextCapture
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
ModbusSimulationMap FormModSim::simulationMap() const
{
    const auto dd = displayDefinition();
    const auto startAddr = dd.PointAddress - 1;
    const auto endAddr = startAddr + dd.Length;

    ModbusSimulationMap result;
    const auto simulationMap = _dataSimulator->simulationMap();
    for(auto&& key : simulationMap.keys())
    {
        if(key.first == dd.PointType &&
           key.second >= startAddr && key.second < endAddr)
        {
            result[key] = simulationMap[key];
        }
    }

    return result;
}


QModbusDataUnit FormModSim::serializeModbusDataUnit(QModbusDataUnit::RegisterType type,
                                                    quint16 startAddress,
                                                    quint16 length) const
{
    QModbusDataUnit dataUnit;

    const auto serverData = _mbMultiServer.data(type, startAddress, length);

    const auto& unit = serverData;

    if (startAddress >= unit.startAddress() &&
        (startAddress + length) <= (unit.startAddress() + unit.valueCount())) {

        int offset = startAddress - unit.startAddress();

        QVector<quint16> values;
        for (int i = 0; i < length; ++i) {
            values.append(unit.value(offset + i));
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
    _dataSimulator->startSimulation(dataDisplayMode(), type, addr, params);
}

void FormModSim::configureModbusDataUnit(QModbusDataUnit::RegisterType type,
                                         quint16 startAddress,
                                         const QVector<quint16>& values) const
{
    QModbusDataUnit unit;
    unit.setRegisterType(type);
    unit.setStartAddress(startAddress);
    unit.setValues(values);
    _mbMultiServer.setData(unit);
}


///
/// \brief FormModSim::descriptionMap
/// \return
///
AddressDescriptionMap FormModSim::descriptionMap() const
{
    return ui->outputWidget->descriptionMap();
}

///
/// \brief FormModSim::setDescription
/// \param type
/// \param addr
/// \param desc
///
void FormModSim::setDescription(QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc)
{
    ui->outputWidget->setDescription(type, addr, desc);
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
void FormModSim::on_lineEditDeviceId_valueChanged(const QVariant&)
{
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
void FormModSim::on_comboBoxModbusPointType_pointTypeChanged(QModbusDataUnit::RegisterType)
{
    onDefinitionChanged();
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
    _mbMultiServer.setDeviceId(dd.DeviceId);
    _mbMultiServer.addUnitMap(formId(), dd.PointType, addr, dd.Length);

    ui->scriptControl->setAddressBase(dd.ZeroBasedAddress ? AddressBase::Base0 : AddressBase::Base1);
    ui->outputWidget->setup(dd, _dataSimulator->simulationMap(), _mbMultiServer.data(dd.PointType, addr, dd.Length));
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
    const auto pointType = ui->comboBoxModbusPointType->currentPointType();
    const auto zeroBasedAddress = displayDefinition().ZeroBasedAddress;
    const auto simAddr = addr - (zeroBasedAddress ? 0 : 1);
    auto simParams = _dataSimulator->simulationParams(pointType, addr);

    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
        {
            ModbusWriteParams params = { addr, value, mode, byteOrder(), zeroBasedAddress };
            DialogWriteCoilRegister dlg(params, simParams, this);
            switch(dlg.exec())
            {
                case QDialog::Accepted:
                    _mbMultiServer.writeRegister(pointType, params);
                break;

                case 2:
                    if(simParams.Mode == SimulationMode::No) _dataSimulator->stopSimulation(pointType, simAddr);
                    else _dataSimulator->startSimulation(mode, pointType, simAddr, simParams);
                break;
            }
        }
        break;

        case QModbusDataUnit::InputRegisters:
        case QModbusDataUnit::HoldingRegisters:
        {
            ModbusWriteParams params = { addr, value, mode, byteOrder(), zeroBasedAddress };
            if(mode == DataDisplayMode::Binary)
            {
                DialogWriteHoldingRegisterBits dlg(params, this);
                if(dlg.exec() == QDialog::Accepted)
                    _mbMultiServer.writeRegister(pointType, params);
            }
            else
            {
                DialogWriteHoldingRegister dlg(params, simParams, this);
                switch(dlg.exec())
                {
                    case QDialog::Accepted:
                        _mbMultiServer.writeRegister(pointType, params);
                    break;

                    case 2:
                        if(simParams.Mode == SimulationMode::No) _dataSimulator->stopSimulation(pointType, simAddr);
                        else _dataSimulator->startSimulation(mode, pointType, simAddr, simParams);
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
/// \brief FormModSim::on_mbDeviceIdChanged
/// \param deviceId
///
void FormModSim::on_mbDeviceIdChanged(quint8 deviceId)
{
    blockSignals(true);
    ui->lineEditDeviceId->setValue(deviceId);
    blockSignals(false);
}

///
/// \brief FormModSim::on_mbConnected
///
void FormModSim::on_mbConnected(const ConnectionDetails&)
{
    updateStatus();
}

///
/// \brief FormModSim::on_mbDisconnected
///
void FormModSim::on_mbDisconnected(const ConnectionDetails&)
{
   updateStatus();
}

///
/// \brief FormModSim::on_mbRequest
/// \param req
/// \param protocol
/// \param transactionId
///
void FormModSim::on_mbRequest(const QModbusRequest& req, ModbusMessage::ProtocolType protocol, int transactionId)
{
    const auto deviceId = ui->lineEditDeviceId->value<int>();
    ui->outputWidget->updateTraffic(req, deviceId, transactionId, protocol);
}

///
/// \brief FormModSim::on_mbResponse
/// \param resp
/// \param protocol
/// \param transactionId
///
void FormModSim::on_mbResponse(const QModbusResponse& resp, ModbusMessage::ProtocolType protocol, int transactionId)
{
    const auto deviceId = ui->lineEditDeviceId->value<int>();
    ui->outputWidget->updateTraffic(resp, deviceId, transactionId, protocol);
}

///
/// \brief FormModSim::on_mbDataChanged
///
void FormModSim::on_mbDataChanged(const QModbusDataUnit&)
{
    const auto dd = displayDefinition();
    const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    ui->outputWidget->updateData(_mbMultiServer.data(dd.PointType, addr, dd.Length));
}

///
/// \brief FormModSim::on_simulationStarted
/// \param type
/// \param addr
///
void FormModSim::on_simulationStarted(QModbusDataUnit::RegisterType type, quint16 addr)
{
    ui->outputWidget->setSimulated(type, addr, true);
}

///
/// \brief FormModSim::on_simulationStopped
/// \param type
/// \param addr
///
void FormModSim::on_simulationStopped(QModbusDataUnit::RegisterType type, quint16 addr)
{
    ui->outputWidget->setSimulated(type, addr, false);
}

///
/// \brief FormModSim::on_dataSimulated
/// \param mode
/// \param type
/// \param addr
/// \param value
///
void FormModSim::on_dataSimulated(DataDisplayMode mode, QModbusDataUnit::RegisterType type, quint16 addr, QVariant value)
{
    const auto dd = displayDefinition();
    const auto pointAddr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    if(type == dd.PointType && addr >= pointAddr && addr <= pointAddr + dd.Length)
    {
        _mbMultiServer.writeRegister(type, { addr, value, mode, byteOrder(), true });
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
