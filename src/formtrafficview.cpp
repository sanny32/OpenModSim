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
#include "formtrafficview.h"
#include "ui_formtrafficview.h"

QVersionNumber FormTrafficView::VERSION = QVersionNumber(1, 15);

///
/// \brief FormTrafficView::FormTrafficView
/// \param num
/// \param parent
///
FormTrafficView::FormTrafficView(int id, ModbusMultiServer& server, DataSimulator* simulator, MainWindow* parent)
    : FormModSim(parent)
    , ui(new Ui::FormTrafficView)
    ,_parent(parent)
    ,_formId(id)
    ,_mbMultiServer(server)
    ,_dataSimulator(simulator)
    ,_verboseLogging(true)
{
    Q_ASSERT(parent != nullptr);
    Q_ASSERT(_dataSimulator != nullptr);

    ui->setupUi(this);
    setWindowTitle(QString("Traffic%1").arg(_formId));
    setWindowIcon(QIcon(":/res/actionShowTraffic.png"));

    ui->lineEditDeviceId->setInputRange(ModbusLimits::slaveRange());
    ui->lineEditDeviceId->setValue(1);
    ui->lineEditDeviceId->setLeadingZeroes(true);
    ui->lineEditDeviceId->setHexButtonVisible(true);
    server.addDeviceId(ui->lineEditDeviceId->value<int>());

    ui->stackedWidget->setCurrentIndex(0);
    ui->outputWidget->enforceTrafficMode();
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

    connect(this, &FormModSim::definitionChanged, this, &FormTrafficView::onDefinitionChanged);
    emit definitionChanged();

    ui->outputWidget->setFocus();
    connect(ui->outputWidget, &OutputTrafficWidget::startTextCaptureError, this, &FormModSim::captureError);
    connect(ui->scriptControl, &JScriptControl::helpContext, this, &FormModSim::helpContextRequested);
    connect(ui->scriptControl, &JScriptControl::scriptStopped, this, &FormModSim::scriptStopped);
    connect(ui->scriptControl, &JScriptControl::consoleMessage, this, &FormModSim::consoleMessage);

    setLogViewState(server.isConnected() ? LogViewState::Running : LogViewState::Unknown);
    connect(ui->statisticWidget, &StatisticWidget::ctrsReseted, ui->outputWidget, &OutputTrafficWidget::clearLogView);
    connect(ui->statisticWidget, &StatisticWidget::logStateChanged, ui->outputWidget, &OutputTrafficWidget::setLogViewState);
    connect(ui->statisticWidget, &StatisticWidget::ctrsReseted, this, &FormModSim::statisticCtrsReseted);
    connect(ui->statisticWidget, &StatisticWidget::logStateChanged, this, &FormModSim::statisticLogStateChanged);

    connect(&_mbMultiServer, &ModbusMultiServer::request, this, &FormTrafficView::on_mbRequest);
    connect(&_mbMultiServer, &ModbusMultiServer::response, this, &FormTrafficView::on_mbResponse);
    connect(&_mbMultiServer, &ModbusMultiServer::connected, this, &FormTrafficView::on_mbConnected);
    connect(&_mbMultiServer, &ModbusMultiServer::disconnected, this, &FormTrafficView::on_mbDisconnected);
    connect(&_mbMultiServer, &ModbusMultiServer::dataChanged, this, &FormTrafficView::on_mbDataChanged);
    connect(&_mbMultiServer, &ModbusMultiServer::definitionsChanged, this, &FormTrafficView::on_mbDefinitionsChanged);

    connect(_dataSimulator, &DataSimulator::simulationStarted, this, &FormTrafficView::on_simulationStarted);
    connect(_dataSimulator, &DataSimulator::simulationStopped, this, &FormTrafficView::on_simulationStopped);
    connect(_dataSimulator, &DataSimulator::dataSimulated, this, &FormTrafficView::on_dataSimulated);

    setupScriptBar();
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
        updateStatus();
    }

    QWidget::changeEvent(e);
}

///
/// \brief FormTrafficView::closeEvent
/// \param event
///
void FormTrafficView::closeEvent(QCloseEvent* event)
{
    const auto deviceId = ui->lineEditDeviceId->value<quint8>();
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
void FormTrafficView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(ui->frameDataDefinition->geometry().contains(event->pos())) {
        emit doubleClicked();
    }

    return QWidget::mouseDoubleClickEvent(event);
}

///
/// \brief FormTrafficView::filename
/// \return
///
QString FormTrafficView::filename() const
{
    return _filename;
}

///
/// \brief FormTrafficView::setFilename
/// \param filename
///
void FormTrafficView::setFilename(const QString& filename)
{
    _filename = filename;
}

///
/// \brief FormTrafficView::data
/// \return
///
QVector<quint16> FormTrafficView::data() const
{
    return ui->outputWidget->data();
}

///
/// \brief FormTrafficView::displayDefinition
/// \return
///
DisplayDefinition FormTrafficView::displayDefinition() const
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
/// \brief FormTrafficView::setDisplayDefinition
/// \param dd
///
void FormTrafficView::setDisplayDefinition(const DisplayDefinition& dd)
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
/// \brief FormTrafficView::displayMode
/// \return
///
DisplayMode FormTrafficView::displayMode() const
{
    if(ui->stackedWidget->currentIndex() == 1)
        return DisplayMode::Script;
    else
        return ui->outputWidget->displayMode();
}

///
/// \brief FormTrafficView::setDisplayMode
/// \param mode
///
void FormTrafficView::setDisplayMode(DisplayMode mode)
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
/// \brief FormTrafficView::dataDisplayMode
/// \return
///
DataDisplayMode FormTrafficView::dataDisplayMode() const
{
    return ui->outputWidget->dataDisplayMode();
}

///
/// \brief FormTrafficView::displayHexAddresses
/// \return
///
bool FormTrafficView::displayHexAddresses() const
{
    return ui->outputWidget->displayHexAddresses();
}

///
/// \brief FormTrafficView::setDisplayHexAddresses
/// \param on
///
void FormTrafficView::setDisplayHexAddresses(bool on)
{
    ui->outputWidget->setDisplayHexAddresses(on);

    const auto defs = _mbMultiServer.getModbusDefinitions();
    ui->lineEditAddress->setInputMode(on ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(defs.AddrSpace, ui->comboBoxAddressBase->currentAddressBase() == AddressBase::Base0));
}

///
/// \brief FormTrafficView::captureMode
///
CaptureMode FormTrafficView::captureMode() const
{
    return ui->outputWidget->captureMode();
}

///
/// \brief FormTrafficView::startTextCapture
/// \param file
///
void FormTrafficView::startTextCapture(const QString& file)
{
    ui->outputWidget->startTextCapture(file);
}

///
/// \brief FormTrafficView::stopTextCapture
///
void FormTrafficView::stopTextCapture()
{
    ui->outputWidget->stopTextCapture();
}

///
/// \brief FormTrafficView::setDataDisplayMode
/// \param mode
///
void FormTrafficView::setDataDisplayMode(DataDisplayMode mode)
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
}

///
/// \brief FormTrafficView::scriptSettings
/// \return
///
ScriptSettings FormTrafficView::scriptSettings() const
{
    return _scriptSettings;
}

///
/// \brief FormTrafficView::setScriptSettings
/// \param ss
///
void FormTrafficView::setScriptSettings(const ScriptSettings& ss)
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
/// \brief FormTrafficView::byteOrder
/// \return
///
ByteOrder FormTrafficView::byteOrder() const
{
    return *ui->outputWidget->byteOrder();
}

///
/// \brief FormTrafficView::setByteOrder
/// \param order
///
void FormTrafficView::setByteOrder(ByteOrder order)
{
    ui->outputWidget->setByteOrder(order);
    emit byteOrderChanged(order);
}

///
/// \brief FormTrafficView::codepage
/// \return
///
QString FormTrafficView::codepage() const
{
    return ui->outputWidget->codepage();
}

///
/// \brief FormTrafficView::setCodepage
/// \param name
///
void FormTrafficView::setCodepage(const QString& name)
{
    ui->outputWidget->setCodepage(name);
    emit codepageChanged(name);
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
/// \brief FormTrafficView::statusColor
/// \return
///
QColor FormTrafficView::statusColor() const
{
    return ui->outputWidget->statusColor();
}

///
/// \brief FormTrafficView::setStatusColor
/// \param clr
///
void FormTrafficView::setStatusColor(const QColor& clr)
{
    ui->outputWidget->setStatusColor(clr);
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
/// \brief FormTrafficView::zoomPercent
/// \return
///
int FormTrafficView::zoomPercent() const
{
    return ui->outputWidget->zoomPercent();
}

///
/// \brief FormTrafficView::setZoomPercent
/// \param zoomPercent
///
void FormTrafficView::setZoomPercent(int zoomPercent)
{
    ui->outputWidget->setZoomPercent(zoomPercent);
}

///
/// \brief FormTrafficView::print
/// \param printer
///
void FormTrafficView::print(QPrinter* printer)
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
/// \brief FormTrafficView::simulationMap
/// \return
///
ModbusSimulationMap2 FormTrafficView::simulationMap() const
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
/// \brief FormTrafficView::serializeModbusDataUnit
/// \param deviceId
/// \param type
/// \param startAddress
/// \param length
/// \return
///
QModbusDataUnit FormTrafficView::serializeModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, quint16 length) const
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
/// \brief FormTrafficView::startSimulation
/// \param type
/// \param addr
/// \param params
///
void FormTrafficView::startSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params)
{
    const auto deviceId = ui->lineEditDeviceId->value<quint8>();
    _dataSimulator->startSimulation(deviceId, type, addr, params);
}

///
/// \brief FormTrafficView::configureModbusDataUnit
/// \param deviceId
/// \param type
/// \param startAddress
/// \param values
///
void FormTrafficView::configureModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, const QVector<quint16>& values) const
{
    QModbusDataUnit unit;
    unit.setRegisterType(type);
    unit.setStartAddress(startAddress);
    unit.setValues(values);
    _mbMultiServer.setData(deviceId, unit);
}


///
/// \brief FormTrafficView::descriptionMap
/// \return
///
AddressDescriptionMap2 FormTrafficView::descriptionMap() const
{
    return ui->outputWidget->descriptionMap();
}

///
/// \brief FormTrafficView::setDescription
/// \param deviceId
/// \param type
/// \param addr
/// \param desc
///
void FormTrafficView::setDescription(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc)
{
    ui->outputWidget->setDescription(deviceId, type, addr, desc);
}

///
/// \brief FormTrafficView::colorMap
/// \return
///
AddressColorMap FormTrafficView::colorMap() const
{
    return ui->outputWidget->colorMap();
}

///
/// \brief FormTrafficView::setColor
/// \param deviceId
/// \param type
/// \param addr
/// \param clr
///
void FormTrafficView::setColor(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QColor& clr)
{
    ui->outputWidget->setColor(deviceId, type, addr, clr);
}

///
/// \brief FormTrafficView::resetCtrs
///
void FormTrafficView::resetCtrs()
{
    ui->statisticWidget->resetCtrs();
}

///
/// \brief FormTrafficView::requestCount
/// \return
///
uint FormTrafficView::requestCount() const
{
    return ui->statisticWidget->numberRequets();
}

///
/// \brief FormTrafficView::responseCount
/// \return
///
uint FormTrafficView::responseCount() const
{
    return ui->statisticWidget->numberResposes();
}

///
/// \brief FormTrafficView::setStatisticCounters
/// \param requests
/// \param responses
///
void FormTrafficView::setStatisticCounters(uint requests, uint responses)
{
    ui->statisticWidget->setCounters(requests, responses);
}

///
/// \brief FormTrafficView::script
/// \return
///
QString FormTrafficView::script() const
{
    return ui->scriptControl->script();
}

///
/// \brief FormTrafficView::setScript
/// \param text
///
void FormTrafficView::setScript(const QString& text)
{
    ui->scriptControl->setScript(text);
}

///
/// \brief FormTrafficView::scriptDocument
/// \return
///
QTextDocument* FormTrafficView::scriptDocument() const
{
    return ui->scriptControl->scriptDocument();
}

///
/// \brief FormTrafficView::setScriptDocument
/// \param document
///
void FormTrafficView::setScriptDocument(QTextDocument* document)
{
    ui->scriptControl->setScriptDocument(document);
}

int FormTrafficView::scriptCursorPosition() const
{
    return ui->scriptControl->cursorPosition();
}

void FormTrafficView::setScriptCursorPosition(int pos)
{
    ui->scriptControl->setCursorPosition(pos);
}

int FormTrafficView::scriptScrollPosition() const
{
    return ui->scriptControl->scrollPosition();
}

void FormTrafficView::setScriptScrollPosition(int pos)
{
    ui->scriptControl->setScrollPosition(pos);
}

///
/// \brief FormTrafficView::searchText
/// \return
///
QString FormTrafficView::searchText() const
{
    return ui->scriptControl->searchText();
}

///
/// \brief FormTrafficView::canRunScript
/// \return
///
bool FormTrafficView::canRunScript() const
{
    return !ui->scriptControl->script().isEmpty() &&
           !ui->scriptControl->isRunning();
}

///
/// \brief FormTrafficView::canStopScript
/// \return
///
bool FormTrafficView::canStopScript() const
{
    return ui->scriptControl->isRunning();
}

///
/// \brief FormTrafficView::canUndo
/// \return
///
bool FormTrafficView::canUndo() const
{
    return ui->scriptControl->canUndo();
}

///
/// \brief FormTrafficView::canRedo
/// \return
///
bool FormTrafficView::canRedo() const
{
    return ui->scriptControl->canRedo();
}

///
/// \brief FormTrafficView::canPaste
/// \return
///
bool FormTrafficView::canPaste() const
{
    return ui->scriptControl->canPaste();
}

///
/// \brief FormTrafficView::runScript
/// \param interval
///
void FormTrafficView::runScript()
{
    emit scriptRunning();
    ui->scriptControl->runScript(_scriptSettings.Mode, _scriptSettings.Interval);
}

///
/// \brief FormTrafficView::stopScript
///
void FormTrafficView::stopScript()
{
    ui->scriptControl->stopScript();
}

///
/// \brief FormTrafficView::logViewState
/// \return
///
LogViewState FormTrafficView::logViewState() const
{
    return ui->statisticWidget->logState();
}

///
/// \brief FormTrafficView::setLogViewState
/// \param state
///
void FormTrafficView::setLogViewState(LogViewState state)
{
    ui->statisticWidget->setLogState(state);
}

///
/// \brief FormTrafficView::parentGeometry
/// \return
///
QRect FormTrafficView::parentGeometry() const
{
    auto wnd = parentWidget();
    return (!wnd->isMaximized() && !wnd->isMinimized()) ? wnd->geometry() : _parentGeometry;
}

///
/// \brief FormTrafficView::setParentGeometry
/// \param geometry
///
void FormTrafficView::setParentGeometry(const QRect& geometry)
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
/// \brief FormTrafficView::show
///
void FormTrafficView::show()
{
    QWidget::show();
    connectEditSlots();

    emit showed();
}

///
/// \brief FormTrafficView::on_lineEditAddress_valueChanged
///
void FormTrafficView::on_lineEditAddress_valueChanged(const QVariant&)
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
/// \brief FormTrafficView::on_lineEditLength_valueChanged
///
void FormTrafficView::on_lineEditLength_valueChanged(const QVariant&)
{
    emit definitionChanged();
}

///
/// \brief FormTrafficView::on_lineEditDeviceId_valueChanged
///
void FormTrafficView::on_lineEditDeviceId_valueChanged(const QVariant& oldValue, const QVariant& newValue)
{
    _mbMultiServer.removeDeviceId(oldValue.toInt());
    _mbMultiServer.addDeviceId(newValue.toInt());

    emit definitionChanged();
}

///
/// \brief FormTrafficView::on_comboBoxAddressBase_addressBaseChanged
/// \param base
///
void FormTrafficView::on_comboBoxAddressBase_addressBaseChanged(AddressBase base)
{
    auto dd = displayDefinition();
    dd.PointAddress = (base == AddressBase::Base1 ? qMax(1, dd.PointAddress + 1) : qMax(0, dd.PointAddress - 1));
    dd.ZeroBasedAddress = (base == AddressBase::Base0);
    setDisplayDefinition(dd);
}

///
/// \brief FormTrafficView::on_comboBoxModbusPointType_pointTypeChanged
///
void FormTrafficView::on_comboBoxModbusPointType_pointTypeChanged(QModbusDataUnit::RegisterType type)
{
    emit definitionChanged();
    emit pointTypeChanged(type);
}

///
/// \brief FormTrafficView::updateStatus
///
void FormTrafficView::updateStatus()
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
/// \brief FormTrafficView::onDefinitionChanged
///
void FormTrafficView::onDefinitionChanged()
{
    updateStatus();

    const auto dd = displayDefinition();
    const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    _mbMultiServer.addUnitMap(formId(), dd.DeviceId, dd.PointType, addr, dd.Length);

    ui->scriptControl->setAddressBase(dd.ZeroBasedAddress ? AddressBase::Base0 : AddressBase::Base1);
    ui->outputWidget->setup(dd, _dataSimulator->simulationMap(), _mbMultiServer.data(dd.DeviceId, dd.PointType, addr, dd.Length));
}

///
/// \brief FormTrafficView::scriptControl
/// \return
///
JScriptControl* FormTrafficView::scriptControl()
{
    return ui->scriptControl;
}

///
/// \brief FormTrafficView::isAutoCompleteEnabled
/// \return
///
bool FormTrafficView::isAutoCompleteEnabled() const
{
    return ui->scriptControl->isAutoCompleteEnabled();
}

///
/// \brief FormTrafficView::enableAutoComplete
/// \param enable
///
void FormTrafficView::enableAutoComplete(bool enable)
{
    ui->scriptControl->enableAutoComplete(enable);
}

///
/// \brief FormTrafficView::setScriptFont
/// \param font
///
void FormTrafficView::setScriptFont(const QFont& font)
{
    ui->scriptControl->setFont(font);
}

///
/// \brief FormTrafficView::on_outputWidget_itemDoubleClicked
/// \param addr
/// \param value
///
void FormTrafficView::on_outputWidget_itemDoubleClicked(quint16 addr, const QVariant& value)
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
/// \brief FormTrafficView::on_mbConnected
///
void FormTrafficView::on_mbConnected(const ConnectionDetails&)
{
    updateStatus();
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
    updateStatus();
    if(!_mbMultiServer.isConnected()) {
        setLogViewState(LogViewState::Unknown);
    }
}

///
/// \brief FormTrafficView::isLoggingRequest
/// \param msgReq
/// \return
///
bool FormTrafficView::isLoggingRequest(QSharedPointer<const ModbusMessage> msgReq) const
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
/// \brief FormTrafficView::on_mbRequest
/// \param msg
///
void FormTrafficView::on_mbRequest(QSharedPointer<const ModbusMessage> msg)
{
    if(_verboseLogging || isLoggingRequest(msg)) {
        ui->statisticWidget->increaseRequests();
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
    if(_verboseLogging || isLoggingRequest(msgReq)) {
        ui->statisticWidget->increaseResponses();
        ui->outputWidget->updateTraffic(msgResp);
    }
}

///
/// \brief FormTrafficView::on_mbDataChanged
///
void FormTrafficView::on_mbDataChanged(quint8 deviceId, const QModbusDataUnit&)
{
    const auto dd = displayDefinition();
    if(deviceId == dd.DeviceId)
    {
        const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
        ui->outputWidget->updateData(_mbMultiServer.data(deviceId, dd.PointType, addr, dd.Length));
    }
}

///
/// \brief FormTrafficView::on_mbDefinitionsChanged
/// \param defs
///
void FormTrafficView::on_mbDefinitionsChanged(const ModbusDefinitions& defs)
{
    const auto dd = displayDefinition();
    const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(defs.AddrSpace, dd.ZeroBasedAddress));
    ui->lineEditLength->setInputRange(ModbusLimits::lengthRange(dd.PointAddress, dd.ZeroBasedAddress, defs.AddrSpace));
    ui->outputWidget->setup(dd, _dataSimulator->simulationMap(), _mbMultiServer.data(dd.DeviceId, dd.PointType, addr, dd.Length));
}

///
/// \brief FormTrafficView::on_simulationStarted
/// \param mode
/// \param deviceId
/// \param type
/// \param addresses
///
void FormTrafficView::on_simulationStarted(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, const QVector<quint16>& addresses)
{
    if(deviceId != ui->lineEditDeviceId->value<quint8>())
        return;

    for(auto&& addr : addresses)
        ui->outputWidget->setSimulated(mode, deviceId, type, addr, true);
}

///
/// \brief FormTrafficView::on_simulationStopped
/// \param mode
/// \param deviceId
/// \param type
/// \param addresses
///
void FormTrafficView::on_simulationStopped(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, const QVector<quint16>& addresses)
{
    if(deviceId != ui->lineEditDeviceId->value<quint8>())
        return;

    for(auto&& addr : addresses)
        ui->outputWidget->setSimulated(mode, deviceId, type, addr, false);
}

///
/// \brief FormTrafficView::on_dataSimulated
/// \param mode
/// \param deviceId
/// \param type
/// \param startAddress
/// \param value
///
void FormTrafficView::on_dataSimulated(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, QVariant value)
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
/// \brief FormTrafficView::setupScriptBar
///
void FormTrafficView::setupScriptBar()
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

    // Interval spinbox (500 вЂ“ 10000 ms)
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
    connect(_actionStopScript, &QAction::triggered, this, &FormTrafficView::stopScript);

    connect(this, &FormModSim::scriptRunning, this, &FormTrafficView::updateScriptBar);
    connect(this, &FormModSim::scriptStopped, this, &FormTrafficView::updateScriptBar);
    connect(ui->scriptControl->scriptDocument(), &QTextDocument::contentsChanged,
            this, &FormTrafficView::updateScriptBar);

    ui->verticalLayout_4->insertWidget(0, _scriptBar);

    updateScriptBar();
}

///
/// \brief FormTrafficView::updateScriptBar
///
void FormTrafficView::updateScriptBar()
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
/// \brief FormTrafficView::connectEditSlots
///
void FormTrafficView::connectEditSlots()
{
    disconnectEditSlots();
    connect(_parent, &MainWindow::selectAll, ui->scriptControl, &JScriptControl::selectAll);
    connect(_parent, &MainWindow::search, ui->scriptControl, &JScriptControl::search);
    connect(_parent, &MainWindow::find, ui->scriptControl, &JScriptControl::showFind);
    connect(_parent, &MainWindow::replace, ui->scriptControl, &JScriptControl::showReplace);
}

///
/// \brief FormTrafficView::disconnectEditSlots
///
void FormTrafficView::disconnectEditSlots()
{
    disconnect(_parent, &MainWindow::selectAll, ui->scriptControl, &JScriptControl::selectAll);
    disconnect(_parent, &MainWindow::search, ui->scriptControl, &JScriptControl::search);
    disconnect(_parent, &MainWindow::find, ui->scriptControl, &JScriptControl::showFind);
    disconnect(_parent, &MainWindow::replace, ui->scriptControl, &JScriptControl::showReplace);
}





