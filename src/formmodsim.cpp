#include <QPainter>
#include <QPalette>
#include <QDateTime>
#include <QHelpEngine>
#include <QHelpContentWidget>
#include <QWidgetAction>
#include "modbuslimits.h"
#include "mainwindow.h"
#include "modbusmessages.h"
#include "datasimulator.h"
#include "dialogwritestatusregister.h"
#include "dialogwriteregister.h"
#include "formmodsim.h"
#include "ui_formmodsim.h"

QVersionNumber FormModSim::VERSION = QVersionNumber(1, 15);

///
/// \brief FormModSim::FormModSim
/// \param num
/// \param parent
///
FormModSim::FormModSim(int id, ModbusMultiServer& server, DataSimulator* simulator, MainWindow* parent)
    : QWidget(parent)
    , ui(new Ui::FormModSim)
    ,_parent(parent)
    ,_formId(id)
    ,_mbMultiServer(server)
    ,_dataSimulator(simulator)
    ,_verboseLogging(true)
{
    Q_ASSERT(parent != nullptr);
    Q_ASSERT(_dataSimulator != nullptr);

    ui->setupUi(this);
    setWindowTitle(QString("ModSim%1").arg(_formId));

    ui->lineEditDeviceId->setInputRange(ModbusLimits::slaveRange());
    ui->lineEditDeviceId->setValue(1);
    ui->lineEditDeviceId->setLeadingZeroes(true);
    ui->lineEditDeviceId->setHexButtonVisible(true);
    server.addDeviceId(ui->lineEditDeviceId->value<int>());

    ui->stackedWidget->setCurrentIndex(0);
    ui->scriptControl->setModbusMultiServer(&_mbMultiServer);
    ui->scriptControl->setByteOrder(ui->outputWidget->byteOrder());
    ui->scriptControl->setScriptSource(windowTitle());

    const auto mbDefs = _mbMultiServer.getModbusDefinitions();

    ui->lineEditAddress->setLeadingZeroes(true);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(mbDefs.AddrSpace, true));
    ui->lineEditAddress->setValue(0);
    ui->lineEditAddress->setHexButtonVisible(true);

    ui->lineEditLength->setInputRange(ModbusLimits::lengthRange(0, true, mbDefs.AddrSpace));
    ui->lineEditLength->setValue(100);
    ui->lineEditLength->setHexButtonVisible(true);

    ui->comboBoxAddressBase->setCurrentAddressBase(AddressBase::Base1);
    ui->comboBoxModbusPointType->setCurrentPointType(QModbusDataUnit::HoldingRegisters);

    connect(this, &FormModSim::definitionChanged, &FormModSim::onDefinitionChanged);
    emit definitionChanged();

    ui->outputWidget->setFocus();
    connect(ui->outputWidget, &OutputWidget::startTextCaptureError, this, &FormModSim::captureError);
    connect(ui->scriptControl, &JScriptControl::helpContext, this, &FormModSim::helpContextRequested);
    connect(ui->scriptControl, &JScriptControl::scriptStopped, this, &FormModSim::scriptStopped);
    connect(ui->scriptControl, &JScriptControl::consoleMessage, this, &FormModSim::consoleMessage);

    setLogViewState(server.isConnected() ? LogViewState::Running : LogViewState::Unknown);
    connect(ui->statisticWidget, &StatisticWidget::ctrsReseted, ui->outputWidget, &OutputWidget::clearLogView);
    connect(ui->statisticWidget, &StatisticWidget::logStateChanged, ui->outputWidget, &OutputWidget::setLogViewState);
    connect(ui->statisticWidget, &StatisticWidget::ctrsReseted, this, &FormModSim::statisticCtrsReseted);
    connect(ui->statisticWidget, &StatisticWidget::logStateChanged, this, &FormModSim::statisticLogStateChanged);

    connect(&_mbMultiServer, &ModbusMultiServer::request, this, &FormModSim::on_mbRequest);
    connect(&_mbMultiServer, &ModbusMultiServer::response, this, &FormModSim::on_mbResponse);
    connect(&_mbMultiServer, &ModbusMultiServer::connected, this, &FormModSim::on_mbConnected);
    connect(&_mbMultiServer, &ModbusMultiServer::disconnected, this, &FormModSim::on_mbDisconnected);
    connect(&_mbMultiServer, &ModbusMultiServer::dataChanged, this, &FormModSim::on_mbDataChanged);
    connect(&_mbMultiServer, &ModbusMultiServer::definitionsChanged, this, &FormModSim::on_mbDefinitionsChanged);

    connect(_dataSimulator, &DataSimulator::simulationStarted, this, &FormModSim::on_simulationStarted);
    connect(_dataSimulator, &DataSimulator::simulationStopped, this, &FormModSim::on_simulationStopped);
    connect(_dataSimulator, &DataSimulator::dataSimulated, this, &FormModSim::on_dataSimulated);

    setupDisplayBar();
    setupScriptBar();
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
void FormModSim::closeEvent(QCloseEvent* event)
{
    const auto deviceId = ui->lineEditDeviceId->value<quint8>();
    _mbMultiServer.removeDeviceId(deviceId);
    _mbMultiServer.removeUnitMap(formId(), deviceId);

    emit closing();
    QWidget::closeEvent(event);
}

///
/// \brief FormModSim::mouseDoubleClickEvent
/// \param event
/// \return
///
void FormModSim::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(ui->frameDataDefinition->geometry().contains(event->pos())) {
        emit doubleClicked();
    }

    return QWidget::mouseDoubleClickEvent(event);
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
    dd.HexViewAddress  = ui->lineEditAddress->hexView();
    dd.HexViewDeviceId = ui->lineEditDeviceId->hexView();
    dd.HexViewLength   = ui->lineEditLength->hexView();
    dd.AddrSpace = _mbMultiServer.getModbusDefinitions().AddrSpace;
    dd.DataViewColumnsDistance = ui->outputWidget->dataViewColumnsDistance();
    dd.LeadingZeros = ui->lineEditDeviceId->leadingZeroes();
    dd.ScriptCfg = _scriptSettings;

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

    const auto defs = _mbMultiServer.getModbusDefinitions();

    ui->lineEditDeviceId->setLeadingZeroes(dd.LeadingZeros);
    ui->lineEditDeviceId->setValue(dd.DeviceId);

    ui->comboBoxAddressBase->blockSignals(true);
    ui->comboBoxAddressBase->setCurrentAddressBase(dd.ZeroBasedAddress ? AddressBase::Base0 : AddressBase::Base1);
    ui->comboBoxAddressBase->blockSignals(false);

    ui->lineEditAddress->blockSignals(true);
    ui->lineEditAddress->setLeadingZeroes(dd.LeadingZeros);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(defs.AddrSpace, dd.ZeroBasedAddress));
    ui->lineEditAddress->setValue(dd.PointAddress);
    ui->lineEditAddress->blockSignals(false);

    ui->lineEditLength->blockSignals(true);
    ui->lineEditLength->setLeadingZeroes(dd.LeadingZeros);
    ui->lineEditLength->setInputRange(ModbusLimits::lengthRange(dd.PointAddress, dd.ZeroBasedAddress, defs.AddrSpace));
    ui->lineEditLength->setValue(dd.Length);
    ui->lineEditLength->blockSignals(false);

    ui->comboBoxModbusPointType->blockSignals(true);
    ui->comboBoxModbusPointType->setCurrentPointType(dd.PointType);
    ui->comboBoxModbusPointType->blockSignals(false);

    ui->outputWidget->setLogViewLimit(dd.LogViewLimit);
    ui->outputWidget->setDataViewColumnsDistance(dd.DataViewColumnsDistance);
    ui->outputWidget->setAutosctollLogView(dd.AutoscrollLog);

    _verboseLogging = dd.VerboseLogging;

    setScriptSettings(dd.ScriptCfg);
    setDisplayHexAddresses(dd.HexAddress);

    ui->lineEditDeviceId->setHexView(dd.HexViewDeviceId);
    ui->lineEditAddress->setHexView(dd.HexViewAddress);
    ui->lineEditLength->setHexView(dd.HexViewLength);

    emit definitionChanged();
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

    const auto defs = _mbMultiServer.getModbusDefinitions();
    ui->lineEditAddress->setInputMode(on ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(defs.AddrSpace, ui->comboBoxAddressBase->currentAddressBase() == AddressBase::Base0));
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
    const auto dd = displayDefinition();
    switch(dd.PointType) {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
            ui->outputWidget->setDataDisplayMode(DataDisplayMode::Binary);
            break;
        case QModbusDataUnit::InputRegisters:
        case QModbusDataUnit::HoldingRegisters:
            ui->outputWidget->setDataDisplayMode(mode);
            break;
        default: break;
    }
    updateDisplayBar();
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
    // Sync toolbar controls if already created
    if (_scriptRunModeCombo)
        _scriptRunModeCombo->setCurrentRunMode(ss.Mode);
    if (_scriptIntervalSpin)
        _scriptIntervalSpin->setValue(static_cast<int>(ss.Interval));
    if (_scriptRunOnStartupCheck)
        _scriptRunOnStartupCheck->setChecked(ss.RunOnStartup);
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
    updateDisplayBar();
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
    if (_ansiMenu) _ansiMenu->selectCodepage(name);
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
/// \brief FormModSim::zoomPercent
/// \return
///
int FormModSim::zoomPercent() const
{
    return ui->outputWidget->zoomPercent();
}

///
/// \brief FormModSim::setZoomPercent
/// \param zoomPercent
///
void FormModSim::setZoomPercent(int zoomPercent)
{
    ui->outputWidget->setZoomPercent(zoomPercent);
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

    const auto textDevIdType = QString(tr("Unit Identifier: %1\nMODBUS Point Type:\n%2")).arg(ui->lineEditDeviceId->text(), ui->comboBoxModbusPointType->currentText());
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
        if(simulationMap[key].Mode == SimulationMode::Disabled)
            continue;

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
    _dataSimulator->startSimulation(deviceId, type, addr, params);
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
/// \brief FormModSim::colorMap
/// \return
///
AddressColorMap FormModSim::colorMap() const
{
    return ui->outputWidget->colorMap();
}

///
/// \brief FormModSim::setColor
/// \param deviceId
/// \param type
/// \param addr
/// \param clr
///
void FormModSim::setColor(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QColor& clr)
{
    ui->outputWidget->setColor(deviceId, type, addr, clr);
}

///
/// \brief FormModSim::resetCtrs
///
void FormModSim::resetCtrs()
{
    ui->statisticWidget->resetCtrs();
}

///
/// \brief FormModSim::requestCount
/// \return
///
uint FormModSim::requestCount() const
{
    return ui->statisticWidget->numberRequets();
}

///
/// \brief FormModSim::responseCount
/// \return
///
uint FormModSim::responseCount() const
{
    return ui->statisticWidget->numberResposes();
}

///
/// \brief FormModSim::setStatisticCounters
/// \param requests
/// \param responses
///
void FormModSim::setStatisticCounters(uint requests, uint responses)
{
    ui->statisticWidget->setCounters(requests, responses);
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
/// \brief FormModSim::scriptDocument
/// \return
///
QTextDocument* FormModSim::scriptDocument() const
{
    return ui->scriptControl->scriptDocument();
}

///
/// \brief FormModSim::setScriptDocument
/// \param document
///
void FormModSim::setScriptDocument(QTextDocument* document)
{
    ui->scriptControl->setScriptDocument(document);
}

int FormModSim::scriptCursorPosition() const
{
    return ui->scriptControl->cursorPosition();
}

void FormModSim::setScriptCursorPosition(int pos)
{
    ui->scriptControl->setCursorPosition(pos);
}

int FormModSim::scriptScrollPosition() const
{
    return ui->scriptControl->scrollPosition();
}

void FormModSim::setScriptScrollPosition(int pos)
{
    ui->scriptControl->setScrollPosition(pos);
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
    emit scriptRunning();
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
/// \brief FormModSim::parentGeometry
/// \return
///
QRect FormModSim::parentGeometry() const
{
    auto wnd = parentWidget();
    return (!wnd->isMaximized() && !wnd->isMinimized()) ? wnd->geometry() : _parentGeometry;
}

///
/// \brief FormModSim::setParentGeometry
/// \param geometry
///
void FormModSim::setParentGeometry(const QRect& geometry)
{
    if(geometry.isValid())
    {
        _parentGeometry = geometry;

        auto wnd = parentWidget();
        if(wnd->geometry() != geometry)
        {
            wnd->setGeometry(geometry);
        }
    }
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
    const auto defs = _mbMultiServer.getModbusDefinitions();
    const bool zeroBased = (ui->comboBoxAddressBase->currentAddressBase() == AddressBase::Base0);
    const int address = ui->lineEditAddress->value<int>();
    const auto lenRange = ModbusLimits::lengthRange(address, zeroBased, defs.AddrSpace);

    ui->lineEditLength->setInputRange(lenRange);
    if(ui->lineEditLength->value<int>() > lenRange.to()) {
        ui->lineEditLength->setValue(lenRange.to());
        ui->lineEditLength->update();
    }

   emit definitionChanged();
}

///
/// \brief FormModSim::on_lineEditLength_valueChanged
///
void FormModSim::on_lineEditLength_valueChanged(const QVariant&)
{
    emit definitionChanged();
}

///
/// \brief FormModSim::on_lineEditDeviceId_valueChanged
///
void FormModSim::on_lineEditDeviceId_valueChanged(const QVariant& oldValue, const QVariant& newValue)
{
    _mbMultiServer.removeDeviceId(oldValue.toInt());
    _mbMultiServer.addDeviceId(newValue.toInt());

    emit definitionChanged();
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
    emit definitionChanged();
    emit pointTypeChanged(type);
    updateDisplayBar();
}

///
/// \brief FormModSim::updateStatus
///
void FormModSim::updateStatus()
{
    const auto dd = displayDefinition();
    if(_mbMultiServer.isConnected())
    {
        const auto defs = _mbMultiServer.getModbusDefinitions();
        const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
        if(addr + dd.Length <= ModbusLimits::addressRange(defs.AddrSpace).to())
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
JScriptControl* FormModSim::scriptControl()
{
    return ui->scriptControl;
}

///
/// \brief FormModSim::isAutoCompleteEnabled
/// \return
///
bool FormModSim::isAutoCompleteEnabled() const
{
    return ui->scriptControl->isAutoCompleteEnabled();
}

///
/// \brief FormModSim::enableAutoComplete
/// \param enable
///
void FormModSim::enableAutoComplete(bool enable)
{
    ui->scriptControl->enableAutoComplete(enable);
}

///
/// \brief FormModSim::setScriptFont
/// \param font
///
void FormModSim::setScriptFont(const QFont& font)
{
    ui->scriptControl->setFont(font);
}

///
/// \brief FormModSim::on_outputWidget_itemDoubleClicked
/// \param addr
/// \param value
///
void FormModSim::on_outputWidget_itemDoubleClicked(quint16 addr, const QVariant& value)
{
    const auto dd = displayDefinition();
    const auto mode = dataDisplayMode();
    const auto deviceId = ui->lineEditDeviceId->value<quint8>();
    const auto pointType = ui->comboBoxModbusPointType->currentPointType();
    const auto zeroBasedAddress = dd.ZeroBasedAddress;
    const auto simAddr = addr - (zeroBasedAddress ? 0 : 1);
    const auto addrSpace = _mbMultiServer.getModbusDefinitions().AddrSpace;

    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
        {
            ModbusWriteParams params;
            params.DeviceId = deviceId;
            params.Address = addr;
            params.Value = value;
            params.DisplayMode = mode;
            params.AddrSpace = addrSpace;
            params.Order = byteOrder();
            params.Codepage = codepage();
            params.ZeroBasedAddress = zeroBasedAddress;
            params.LeadingZeros = dd.LeadingZeros;
            params.Server = &_mbMultiServer;

            DialogWriteStatusRegister dlg(params, pointType, displayHexAddresses(), _dataSimulator, _parent);
            if(dlg.exec() == QDialog::Accepted)
                _mbMultiServer.writeRegister(pointType, params);
        }
        break;

        case QModbusDataUnit::InputRegisters:
        case QModbusDataUnit::HoldingRegisters:
        {
            ModbusWriteParams params;
            params.DeviceId = deviceId;
            params.Address = addr;
            params.Value = value;
            params.DisplayMode = mode;
            params.AddrSpace = addrSpace;
            params.Order = byteOrder();
            params.Codepage = codepage();
            params.ZeroBasedAddress = zeroBasedAddress;
            params.LeadingZeros = dd.LeadingZeros;
            params.Server = &_mbMultiServer;

            DialogWriteRegister dlg(params, pointType, displayHexAddresses(), _dataSimulator, _parent);
            if(dlg.exec() == QDialog::Accepted)
                _mbMultiServer.writeRegister(pointType, params);
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
/// \param msgReq
/// \return
///
bool FormModSim::isLoggingRequest(QSharedPointer<const ModbusMessage> msgReq) const
{
    if(!msgReq)
        return false;

    const auto dd = displayDefinition();
    if(dd.DeviceId != msgReq->deviceId())
        return false;

    const auto startAddress = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);

    switch(msgReq->functionCode()) {
        case QModbusPdu::ReadCoils: {
            auto req = reinterpret_cast<const ReadCoilsRequest*>(msgReq.get());
            return (req->startAddress() >= startAddress) && (dd.PointType == QModbusDataUnit::Coils);
        }
        case QModbusPdu::ReadDiscreteInputs: {
            auto req = reinterpret_cast<const ReadDiscreteInputsRequest*>(msgReq.get());
            return (req->startAddress() >= startAddress) && (dd.PointType == QModbusDataUnit::DiscreteInputs);
        }
        case QModbusPdu::ReadHoldingRegisters: {
            auto req = reinterpret_cast<const ReadHoldingRegistersRequest*>(msgReq.get());
            return (req->startAddress() >= startAddress) && (dd.PointType == QModbusDataUnit::HoldingRegisters);
        }
        case QModbusPdu::ReadInputRegisters: {
            auto req = reinterpret_cast<const ReadInputRegistersRequest*>(msgReq.get());
            return (req->startAddress() >= startAddress) && (dd.PointType == QModbusDataUnit::InputRegisters);
        }
        case QModbusPdu::WriteSingleCoil: {
            auto req = reinterpret_cast<const WriteSingleCoilRequest*>(msgReq.get());
            return (req->address() >= startAddress) && (dd.PointType == QModbusDataUnit::Coils);
        }
        case QModbusPdu::WriteSingleRegister: {
            auto req = reinterpret_cast<const WriteSingleRegisterRequest*>(msgReq.get());
            return (req->address() >= startAddress) && (dd.PointType == QModbusDataUnit::HoldingRegisters);
        }
        case QModbusPdu::WriteMultipleCoils: {
            auto req = reinterpret_cast<const WriteMultipleCoilsRequest*>(msgReq.get());
            return (req->startAddress() >= startAddress) && (dd.PointType == QModbusDataUnit::Coils);
        }
        case QModbusPdu::WriteMultipleRegisters: {
            auto req = reinterpret_cast<const WriteMultipleRegistersRequest*>(msgReq.get());
            return (req->startAddress() >= startAddress) && (dd.PointType == QModbusDataUnit::HoldingRegisters);
        }
        case QModbusPdu::MaskWriteRegister: {
            auto req = reinterpret_cast<const MaskWriteRegisterRequest*>(msgReq.get());
            return (req->address() >= startAddress) && (dd.PointType == QModbusDataUnit::HoldingRegisters);
        }
        case QModbusPdu::ReadWriteMultipleRegisters: {
            auto req = reinterpret_cast<const ReadWriteMultipleRegistersRequest*>(msgReq.get());
            return ((req->readStartAddress() >= startAddress) || (req->writeStartAddress() >= startAddress)) && (dd.PointType == QModbusDataUnit::HoldingRegisters);
        }
        default:
            return true;
    }
}

///
/// \brief FormModSim::on_mbRequest
/// \param msg
///
void FormModSim::on_mbRequest(QSharedPointer<const ModbusMessage> msg)
{
    if(_verboseLogging || isLoggingRequest(msg)) {
        ui->statisticWidget->increaseRequests();
        ui->outputWidget->updateTraffic(msg);
    }
}

///
/// \brief FormModSim::on_mbResponse
/// \param msgReq
/// \param msgResp
///
void FormModSim::on_mbResponse(QSharedPointer<const ModbusMessage> msgReq, QSharedPointer<const ModbusMessage> msgResp)
{
    if(_verboseLogging || isLoggingRequest(msgReq)) {
        ui->statisticWidget->increaseResponses();
        ui->outputWidget->updateTraffic(msgResp);
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
/// \brief FormModSim::on_mbDefinitionsChanged
/// \param defs
///
void FormModSim::on_mbDefinitionsChanged(const ModbusDefinitions& defs)
{
    const auto dd = displayDefinition();
    const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(defs.AddrSpace, dd.ZeroBasedAddress));
    ui->lineEditLength->setInputRange(ModbusLimits::lengthRange(dd.PointAddress, dd.ZeroBasedAddress, defs.AddrSpace));
    ui->outputWidget->setup(dd, _dataSimulator->simulationMap(), _mbMultiServer.data(dd.DeviceId, dd.PointType, addr, dd.Length));
}

///
/// \brief FormModSim::on_simulationStarted
/// \param mode
/// \param deviceId
/// \param type
/// \param addresses
///
void FormModSim::on_simulationStarted(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, const QVector<quint16>& addresses)
{
    if(deviceId != ui->lineEditDeviceId->value<quint8>())
        return;

    for(auto&& addr : addresses)
        ui->outputWidget->setSimulated(mode, deviceId, type, addr, true);
}

///
/// \brief FormModSim::on_simulationStopped
/// \param mode
/// \param deviceId
/// \param type
/// \param addresses
///
void FormModSim::on_simulationStopped(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, const QVector<quint16>& addresses)
{
    if(deviceId != ui->lineEditDeviceId->value<quint8>())
        return;

    for(auto&& addr : addresses)
        ui->outputWidget->setSimulated(mode, deviceId, type, addr, false);
}

///
/// \brief FormModSim::on_dataSimulated
/// \param mode
/// \param deviceId
/// \param type
/// \param startAddress
/// \param value
///
void FormModSim::on_dataSimulated(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, QVariant value)
{
    const auto dd = displayDefinition();
    const auto pointAddr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    if(deviceId == dd.DeviceId && type == dd.PointType && startAddress >= pointAddr && startAddress <= pointAddr + dd.Length)
    {
        const ModbusWriteParams params = { dd.DeviceId, startAddress, value, mode, dd.AddrSpace, byteOrder(), codepage(), true };
        _mbMultiServer.writeRegister(type, params);
    }
}

///
/// \brief FormModSim::setupScriptBar
///
void FormModSim::setupScriptBar()
{
    _scriptBar = new QToolBar(this);
    _scriptBar->setIconSize(QSize(16, 16));

    _scriptRunModeCombo = new RunModeComboBox(_scriptBar);
    _scriptRunModeCombo->setCurrentRunMode(_scriptSettings.Mode);
    connect(_scriptRunModeCombo, &RunModeComboBox::runModeChanged, this, [this](RunMode mode) {
        _scriptSettings.Mode = mode;
    });

    auto modeAction = new QWidgetAction(_scriptBar);
    modeAction->setDefaultWidget(_scriptRunModeCombo);
    _scriptBar->addAction(modeAction);

    // Interval spinbox (500 – 10000 ms)
    _scriptIntervalSpin = new QSpinBox(_scriptBar);
    _scriptIntervalSpin->setRange(500, 10000);
    _scriptIntervalSpin->setSingleStep(100);
    _scriptIntervalSpin->setSuffix(tr(" ms"));
    _scriptIntervalSpin->setValue(_scriptSettings.Interval);
    _scriptIntervalSpin->setFixedWidth(90);
    connect(_scriptIntervalSpin, &QSpinBox::valueChanged, this, [this](int value) {
        _scriptSettings.Interval = static_cast<uint>(value);
    });
    auto intervalAction = new QWidgetAction(_scriptBar);
    intervalAction->setDefaultWidget(_scriptIntervalSpin);
    _scriptBar->addAction(intervalAction);

    // Run on startup checkbox
    _scriptRunOnStartupCheck = new QCheckBox(tr("Run on startup"), _scriptBar);
    _scriptRunOnStartupCheck->setChecked(_scriptSettings.RunOnStartup);
    connect(_scriptRunOnStartupCheck, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        _scriptSettings.RunOnStartup = (state == Qt::Checked);
    });
    auto startupAction = new QWidgetAction(_scriptBar);
    startupAction->setDefaultWidget(_scriptRunOnStartupCheck);
    _scriptBar->addAction(startupAction);

    _scriptBar->addSeparator();

    _actionRunScript = _scriptBar->addAction(QIcon(":/res/actionRunScript.png"), tr("Run Script"));
    qobject_cast<QToolButton*>(_scriptBar->widgetForAction(_actionRunScript))->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(_actionRunScript, &QAction::triggered, this, [this]() {
        runScript();
    });

    _actionStopScript = _scriptBar->addAction(QIcon(":/res/actionStopScript.png"), tr("Stop Script"));
    qobject_cast<QToolButton*>(_scriptBar->widgetForAction(_actionStopScript))->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(_actionStopScript, &QAction::triggered, this, &FormModSim::stopScript);

    connect(this, &FormModSim::scriptRunning, this, &FormModSim::updateScriptBar);
    connect(this, &FormModSim::scriptStopped, this, &FormModSim::updateScriptBar);
    connect(ui->scriptControl->scriptDocument(), &QTextDocument::contentsChanged,
            this, &FormModSim::updateScriptBar);

    ui->verticalLayout_4->insertWidget(0, _scriptBar);

    updateScriptBar();
}

///
/// \brief FormModSim::updateScriptBar
///
void FormModSim::updateScriptBar()
{
    if (!_scriptBar) return;

    const bool running = canStopScript();
    _actionRunScript->setEnabled(canRunScript());
    _actionStopScript->setEnabled(running);
    if (_scriptRunModeCombo) _scriptRunModeCombo->setEnabled(!running);
    if (_scriptIntervalSpin) _scriptIntervalSpin->setEnabled(!running);
    if (_scriptRunOnStartupCheck) _scriptRunOnStartupCheck->setEnabled(!running);
}

///
/// \brief FormModSim::setupDisplayBar
///
void FormModSim::setupDisplayBar()
{
    _displayBar = new QToolBar(this);
    _displayBar->setIconSize(QSize(16, 16));

    auto group = new QActionGroup(_displayBar);
    group->setExclusive(true);

    auto addModeAction = [&](DataDisplayMode mode, const QString& icon, const QString& text)
    {
        auto action = _displayBar->addAction(QIcon(icon), text);
        action->setCheckable(true);
        group->addAction(action);
        _displayModeActions[mode] = action;
        connect(action, &QAction::triggered, this, [this, mode](bool checked) {
            if (checked) setDataDisplayMode(mode);
        });
        return action;
    };

    addModeAction(DataDisplayMode::Binary,        ":/res/actionBinary.png",       tr("Binary"));
    addModeAction(DataDisplayMode::Hex,           ":/res/actionHex.png",          tr("Hex"));
    auto ansiAction = addModeAction(DataDisplayMode::Ansi, ":/res/actionAnsi.png", tr("Ansi"));

    _ansiMenu = new AnsiMenu(this);
    connect(_ansiMenu, &AnsiMenu::codepageSelected, this, &FormModSim::setCodepage);
    ansiAction->setMenu(_ansiMenu);
    qobject_cast<QToolButton*>(_displayBar->widgetForAction(ansiAction))->setPopupMode(QToolButton::DelayedPopup);

    _displayBar->addSeparator();
    addModeAction(DataDisplayMode::Int16,         ":/res/actionInt16.png",        tr("16-bit Integer"));
    addModeAction(DataDisplayMode::UInt16,        ":/res/actionUInt16.png",       tr("Unsigned 16-bit Integer"));

    _displayBar->addSeparator();
    addModeAction(DataDisplayMode::Int32,         ":/res/actionInt32.png",        tr("32-bit Integer (MSRF)"));
    addModeAction(DataDisplayMode::SwappedInt32,  ":/res/actionSwappedInt32.png", tr("32-bit Integer (LSRF)"));

    _displayBar->addSeparator();
    addModeAction(DataDisplayMode::UInt32,        ":/res/actionUInt32.png",       tr("Unsigned 32-bit Integer (MSRF)"));
    addModeAction(DataDisplayMode::SwappedUInt32, ":/res/actionSwappedUInt32.png",tr("Unsigned 32-bit Integer (LSRF)"));

    _displayBar->addSeparator();
    addModeAction(DataDisplayMode::Int64,         ":/res/actionInt64.png",        tr("64-bit Integer (MSRF)"));
    addModeAction(DataDisplayMode::SwappedInt64,  ":/res/actionSwappedInt64.png", tr("64-bit Integer (LSRF)"));

    _displayBar->addSeparator();
    addModeAction(DataDisplayMode::UInt64,        ":/res/actionUInt64.png",       tr("Unsigned 64-bit Integer (MSRF)"));
    addModeAction(DataDisplayMode::SwappedUInt64, ":/res/actionSwappedUInt64.png",tr("Unsigned 64-bit Integer (LSRF)"));

    _displayBar->addSeparator();
    addModeAction(DataDisplayMode::FloatingPt,    ":/res/actionFloatingPt.png",   tr("Float (MSRF)"));
    addModeAction(DataDisplayMode::SwappedFP,     ":/res/actionSwappedFP.png",    tr("Float (LSRF)"));

    _displayBar->addSeparator();
    addModeAction(DataDisplayMode::DblFloat,      ":/res/actionDblFloat.png",     tr("Double (MSRF)"));
    addModeAction(DataDisplayMode::SwappedDbl,    ":/res/actionSwappedDbl.png",   tr("Double (LSRF)"));

    _displayBar->addSeparator();
    _actionSwapBytes = _displayBar->addAction(QIcon(":/res/actionSwapBytes.png"), tr("Swap Bytes"));
    _actionSwapBytes->setCheckable(true);
    connect(_actionSwapBytes, &QAction::triggered, this, [this](bool checked) {
        setByteOrder(checked ? ByteOrder::Swapped : ByteOrder::Direct);
    });

    ui->verticalLayout_2->insertWidget(0, _displayBar);

    updateDisplayBar();
}

///
/// \brief FormModSim::updateDisplayBar
///
void FormModSim::updateDisplayBar()
{
    if (!_displayBar) return;

    const auto ddm = dataDisplayMode();
    const auto dd = displayDefinition();
    const bool coilType = (dd.PointType == QModbusDataUnit::Coils ||
                           dd.PointType == QModbusDataUnit::DiscreteInputs);

    // Check the matching mode action
    const auto it = _displayModeActions.find(ddm);
    if (it != _displayModeActions.end())
        it.value()->setChecked(true);

    // Enable/disable non-binary actions for coil/discrete types
    for (auto action : _displayModeActions) {
        action->setEnabled(!coilType || action == _displayModeActions.value(DataDisplayMode::Binary));
    }

    // Sync swap bytes
    if (_actionSwapBytes) {
        _actionSwapBytes->setChecked(byteOrder() == ByteOrder::Swapped);
        _actionSwapBytes->setEnabled(!coilType);
    }

    // Sync ANSI codepage menu
    if (_ansiMenu) _ansiMenu->selectCodepage(codepage());
}

///
/// \brief FormModSim::connectEditSlots
///
void FormModSim::connectEditSlots()
{
    disconnectEditSlots();
    connect(_parent, &MainWindow::selectAll, ui->scriptControl, &JScriptControl::selectAll);
    connect(_parent, &MainWindow::search, ui->scriptControl, &JScriptControl::search);
    connect(_parent, &MainWindow::find, ui->scriptControl, &JScriptControl::showFind);
    connect(_parent, &MainWindow::replace, ui->scriptControl, &JScriptControl::showReplace);
}

///
/// \brief FormModSim::disconnectEditSlots
///
void FormModSim::disconnectEditSlots()
{
    disconnect(_parent, &MainWindow::selectAll, ui->scriptControl, &JScriptControl::selectAll);
    disconnect(_parent, &MainWindow::search, ui->scriptControl, &JScriptControl::search);
    disconnect(_parent, &MainWindow::find, ui->scriptControl, &JScriptControl::showFind);
    disconnect(_parent, &MainWindow::replace, ui->scriptControl, &JScriptControl::showReplace);
}
