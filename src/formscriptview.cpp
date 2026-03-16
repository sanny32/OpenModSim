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
#include "formscriptview.h"
#include "ui_formscriptview.h"

QVersionNumber FormScriptView::VERSION = QVersionNumber(1, 15);

///
/// \brief FormScriptView::FormScriptView
/// \param num
/// \param parent
///
FormScriptView::FormScriptView(int id, ModbusMultiServer& server, DataSimulator* simulator, MainWindow* parent)
    : FormModSim(parent)
    , ui(new Ui::FormScriptView)
    ,_parent(parent)
    ,_formId(id)
    ,_mbMultiServer(server)
    ,_dataSimulator(simulator)
    ,_verboseLogging(true)
{
    Q_ASSERT(parent != nullptr);
    Q_ASSERT(_dataSimulator != nullptr);

    ui->setupUi(this);
    setWindowTitle(QString("Script%1").arg(_formId));
    setWindowIcon(QIcon(":/res/actionShowScript.png"));

    ui->lineEditDeviceId->setInputRange(ModbusLimits::slaveRange());
    ui->lineEditDeviceId->setValue(1);
    ui->lineEditDeviceId->setLeadingZeroes(true);
    ui->lineEditDeviceId->setHexButtonVisible(true);
    server.addDeviceId(ui->lineEditDeviceId->value<int>());

    ui->stackedWidget->setCurrentIndex(1);
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

    connect(this, &FormModSim::definitionChanged, this, &FormScriptView::onDefinitionChanged);
    emit definitionChanged();

    ui->outputWidget->setFocus();
    connect(ui->outputWidget, &OutputTrafficWidget::startTextCaptureError, this, &FormModSim::captureError);
    connect(ui->scriptControl, &JScriptControl::helpContext, this, &FormModSim::helpContextRequested);
    connect(ui->scriptControl, &JScriptControl::scriptStopped, this, &FormModSim::scriptStopped);
    connect(ui->scriptControl, &JScriptControl::consoleMessage, this, &FormModSim::consoleMessage);

    setLogViewState(server.isConnected() ? LogViewState::Running : LogViewState::Unknown);

    connect(&_mbMultiServer, &ModbusMultiServer::request, this, &FormScriptView::on_mbRequest);
    connect(&_mbMultiServer, &ModbusMultiServer::response, this, &FormScriptView::on_mbResponse);
    connect(&_mbMultiServer, &ModbusMultiServer::connected, this, &FormScriptView::on_mbConnected);
    connect(&_mbMultiServer, &ModbusMultiServer::disconnected, this, &FormScriptView::on_mbDisconnected);
    connect(&_mbMultiServer, &ModbusMultiServer::dataChanged, this, &FormScriptView::on_mbDataChanged);
    connect(&_mbMultiServer, &ModbusMultiServer::definitionsChanged, this, &FormScriptView::on_mbDefinitionsChanged);

    connect(_dataSimulator, &DataSimulator::simulationStarted, this, &FormScriptView::on_simulationStarted);
    connect(_dataSimulator, &DataSimulator::simulationStopped, this, &FormScriptView::on_simulationStopped);
    connect(_dataSimulator, &DataSimulator::dataSimulated, this, &FormScriptView::on_dataSimulated);

    setupScriptBar();
}

///
/// \brief FormScriptView::~FormScriptView
///
FormScriptView::~FormScriptView()
{
    delete ui;
}

void FormScriptView::saveSettings(QSettings& out) const
{
    out << const_cast<FormScriptView*>(this);
}

void FormScriptView::loadSettings(QSettings& in)
{
    in >> this;
}

void FormScriptView::saveXml(QXmlStreamWriter& xml) const
{
    xml << const_cast<FormScriptView*>(this);
}

void FormScriptView::loadXml(QXmlStreamReader& xml)
{
    xml >> this;
}

///
/// \brief FormScriptView::changeEvent
/// \param e
///
void FormScriptView::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        updateStatus();
    }

    QWidget::changeEvent(e);
}

///
/// \brief FormScriptView::closeEvent
/// \param event
///
void FormScriptView::closeEvent(QCloseEvent* event)
{
    const auto deviceId = ui->lineEditDeviceId->value<quint8>();
    _mbMultiServer.removeDeviceId(deviceId);
    _mbMultiServer.removeUnitMap(formId(), deviceId);

    emit closing();
    QWidget::closeEvent(event);
}

///
/// \brief FormScriptView::mouseDoubleClickEvent
/// \param event
/// \return
///
void FormScriptView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(ui->frameDataDefinition->geometry().contains(event->pos())) {
        emit doubleClicked();
    }

    return QWidget::mouseDoubleClickEvent(event);
}

///
/// \brief FormScriptView::filename
/// \return
///
QString FormScriptView::filename() const
{
    return _filename;
}

///
/// \brief FormScriptView::setFilename
/// \param filename
///
void FormScriptView::setFilename(const QString& filename)
{
    _filename = filename;
}

///
/// \brief FormScriptView::data
/// \return
///
QVector<quint16> FormScriptView::data() const
{
    return ui->outputWidget->data();
}

///
/// \brief FormScriptView::displayDefinition
/// \return
///
DisplayDefinition FormScriptView::displayDefinition() const
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
/// \brief FormScriptView::setDisplayDefinition
/// \param dd
///
void FormScriptView::setDisplayDefinition(const DisplayDefinition& dd)
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
/// \brief FormScriptView::displayMode
/// \return
///
DisplayMode FormScriptView::displayMode() const
{
    if(ui->stackedWidget->currentIndex() == 1)
        return DisplayMode::Script;
    else
        return ui->outputWidget->displayMode();
}

///
/// \brief FormScriptView::setDisplayMode
/// \param mode
///
void FormScriptView::setDisplayMode(DisplayMode mode)
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
/// \brief FormScriptView::dataDisplayMode
/// \return
///
DataDisplayMode FormScriptView::dataDisplayMode() const
{
    return ui->outputWidget->dataDisplayMode();
}

///
/// \brief FormScriptView::displayHexAddresses
/// \return
///
bool FormScriptView::displayHexAddresses() const
{
    return ui->outputWidget->displayHexAddresses();
}

///
/// \brief FormScriptView::setDisplayHexAddresses
/// \param on
///
void FormScriptView::setDisplayHexAddresses(bool on)
{
    ui->outputWidget->setDisplayHexAddresses(on);

    const auto defs = _mbMultiServer.getModbusDefinitions();
    ui->lineEditAddress->setInputMode(on ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(defs.AddrSpace, ui->comboBoxAddressBase->currentAddressBase() == AddressBase::Base0));
}

///
/// \brief FormScriptView::captureMode
///
CaptureMode FormScriptView::captureMode() const
{
    return ui->outputWidget->captureMode();
}

///
/// \brief FormScriptView::startTextCapture
/// \param file
///
void FormScriptView::startTextCapture(const QString& file)
{
    ui->outputWidget->startTextCapture(file);
}

///
/// \brief FormScriptView::stopTextCapture
///
void FormScriptView::stopTextCapture()
{
    ui->outputWidget->stopTextCapture();
}

///
/// \brief FormScriptView::setDataDisplayMode
/// \param mode
///
void FormScriptView::setDataDisplayMode(DataDisplayMode mode)
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
/// \brief FormScriptView::scriptSettings
/// \return
///
ScriptSettings FormScriptView::scriptSettings() const
{
    return _scriptSettings;
}

///
/// \brief FormScriptView::setScriptSettings
/// \param ss
///
void FormScriptView::setScriptSettings(const ScriptSettings& ss)
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
/// \brief FormScriptView::byteOrder
/// \return
///
ByteOrder FormScriptView::byteOrder() const
{
    return *ui->outputWidget->byteOrder();
}

///
/// \brief FormScriptView::setByteOrder
/// \param order
///
void FormScriptView::setByteOrder(ByteOrder order)
{
    ui->outputWidget->setByteOrder(order);
    emit byteOrderChanged(order);
}

///
/// \brief FormScriptView::codepage
/// \return
///
QString FormScriptView::codepage() const
{
    return ui->outputWidget->codepage();
}

///
/// \brief FormScriptView::setCodepage
/// \param name
///
void FormScriptView::setCodepage(const QString& name)
{
    ui->outputWidget->setCodepage(name);
    emit codepageChanged(name);
}

///
/// \brief FormScriptView::backgroundColor
/// \return
///
QColor FormScriptView::backgroundColor() const
{
    return ui->outputWidget->backgroundColor();
}

///
/// \brief FormScriptView::setBackgroundColor
/// \param clr
///
void FormScriptView::setBackgroundColor(const QColor& clr)
{
    ui->outputWidget->setBackgroundColor(clr);
}

///
/// \brief FormScriptView::foregroundColor
/// \return
///
QColor FormScriptView::foregroundColor() const
{
    return ui->outputWidget->foregroundColor();
}

///
/// \brief FormScriptView::setForegroundColor
/// \param clr
///
void FormScriptView::setForegroundColor(const QColor& clr)
{
    ui->outputWidget->setForegroundColor(clr);
}

///
/// \brief FormScriptView::statusColor
/// \return
///
QColor FormScriptView::statusColor() const
{
    return ui->outputWidget->statusColor();
}

///
/// \brief FormScriptView::setStatusColor
/// \param clr
///
void FormScriptView::setStatusColor(const QColor& clr)
{
    ui->outputWidget->setStatusColor(clr);
}

///
/// \brief FormScriptView::font
/// \return
///
QFont FormScriptView::font() const
{
   return ui->outputWidget->font();
}

///
/// \brief FormScriptView::setFont
/// \param font
///
void FormScriptView::setFont(const QFont& font)
{
    ui->outputWidget->setFont(font);
}

///
/// \brief FormScriptView::zoomPercent
/// \return
///
int FormScriptView::zoomPercent() const
{
    return ui->outputWidget->zoomPercent();
}

///
/// \brief FormScriptView::setZoomPercent
/// \param zoomPercent
///
void FormScriptView::setZoomPercent(int zoomPercent)
{
    ui->outputWidget->setZoomPercent(zoomPercent);
}

///
/// \brief FormScriptView::print
/// \param printer
///
void FormScriptView::print(QPrinter* printer)
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
/// \brief FormScriptView::simulationMap
/// \return
///
ModbusSimulationMap2 FormScriptView::simulationMap() const
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
/// \brief FormScriptView::serializeModbusDataUnit
/// \param deviceId
/// \param type
/// \param startAddress
/// \param length
/// \return
///
QModbusDataUnit FormScriptView::serializeModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, quint16 length) const
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
/// \brief FormScriptView::startSimulation
/// \param type
/// \param addr
/// \param params
///
void FormScriptView::startSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params)
{
    const auto deviceId = ui->lineEditDeviceId->value<quint8>();
    _dataSimulator->startSimulation(deviceId, type, addr, params);
}

///
/// \brief FormScriptView::configureModbusDataUnit
/// \param deviceId
/// \param type
/// \param startAddress
/// \param values
///
void FormScriptView::configureModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, const QVector<quint16>& values) const
{
    QModbusDataUnit unit;
    unit.setRegisterType(type);
    unit.setStartAddress(startAddress);
    unit.setValues(values);
    _mbMultiServer.setData(deviceId, unit);
}


///
/// \brief FormScriptView::descriptionMap
/// \return
///
AddressDescriptionMap2 FormScriptView::descriptionMap() const
{
    return ui->outputWidget->descriptionMap();
}

///
/// \brief FormScriptView::setDescription
/// \param deviceId
/// \param type
/// \param addr
/// \param desc
///
void FormScriptView::setDescription(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc)
{
    ui->outputWidget->setDescription(deviceId, type, addr, desc);
}

///
/// \brief FormScriptView::colorMap
/// \return
///
AddressColorMap FormScriptView::colorMap() const
{
    return ui->outputWidget->colorMap();
}

///
/// \brief FormScriptView::setColor
/// \param deviceId
/// \param type
/// \param addr
/// \param clr
///
void FormScriptView::setColor(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QColor& clr)
{
    ui->outputWidget->setColor(deviceId, type, addr, clr);
}

///
/// \brief FormScriptView::resetCtrs
///
void FormScriptView::resetCtrs()
{
    _requestCount = 0;
    _responseCount = 0;
    ui->outputWidget->clearLogView();
    emit statisticCtrsReseted();
}

///
/// \brief FormScriptView::requestCount
/// \return
///
uint FormScriptView::requestCount() const
{
    return _requestCount;
}

///
/// \brief FormScriptView::responseCount
/// \return
///
uint FormScriptView::responseCount() const
{
    return _responseCount;
}

///
/// \brief FormScriptView::setStatisticCounters
/// \param requests
/// \param responses
///
void FormScriptView::setStatisticCounters(uint requests, uint responses)
{
    _requestCount = requests;
    _responseCount = responses;
}

///
/// \brief FormScriptView::script
/// \return
///
QString FormScriptView::script() const
{
    return ui->scriptControl->script();
}

///
/// \brief FormScriptView::setScript
/// \param text
///
void FormScriptView::setScript(const QString& text)
{
    ui->scriptControl->setScript(text);
}

///
/// \brief FormScriptView::scriptDocument
/// \return
///
QTextDocument* FormScriptView::scriptDocument() const
{
    return ui->scriptControl->scriptDocument();
}

///
/// \brief FormScriptView::setScriptDocument
/// \param document
///
void FormScriptView::setScriptDocument(QTextDocument* document)
{
    ui->scriptControl->setScriptDocument(document);
}

int FormScriptView::scriptCursorPosition() const
{
    return ui->scriptControl->cursorPosition();
}

void FormScriptView::setScriptCursorPosition(int pos)
{
    ui->scriptControl->setCursorPosition(pos);
}

int FormScriptView::scriptScrollPosition() const
{
    return ui->scriptControl->scrollPosition();
}

void FormScriptView::setScriptScrollPosition(int pos)
{
    ui->scriptControl->setScrollPosition(pos);
}

///
/// \brief FormScriptView::searchText
/// \return
///
QString FormScriptView::searchText() const
{
    return ui->scriptControl->searchText();
}

///
/// \brief FormScriptView::canRunScript
/// \return
///
bool FormScriptView::canRunScript() const
{
    return !ui->scriptControl->script().isEmpty() &&
           !ui->scriptControl->isRunning();
}

///
/// \brief FormScriptView::canStopScript
/// \return
///
bool FormScriptView::canStopScript() const
{
    return ui->scriptControl->isRunning();
}

///
/// \brief FormScriptView::canUndo
/// \return
///
bool FormScriptView::canUndo() const
{
    return ui->scriptControl->canUndo();
}

///
/// \brief FormScriptView::canRedo
/// \return
///
bool FormScriptView::canRedo() const
{
    return ui->scriptControl->canRedo();
}

///
/// \brief FormScriptView::canPaste
/// \return
///
bool FormScriptView::canPaste() const
{
    return ui->scriptControl->canPaste();
}

///
/// \brief FormScriptView::runScript
/// \param interval
///
void FormScriptView::runScript()
{
    emit scriptRunning();
    ui->scriptControl->runScript(_scriptSettings.Mode, _scriptSettings.Interval);
}

///
/// \brief FormScriptView::stopScript
///
void FormScriptView::stopScript()
{
    ui->scriptControl->stopScript();
}

///
/// \brief FormScriptView::logViewState
/// \return
///
LogViewState FormScriptView::logViewState() const
{
    return _logViewState;
}

///
/// \brief FormScriptView::setLogViewState
/// \param state
///
void FormScriptView::setLogViewState(LogViewState state)
{
    if(_logViewState == state)
        return;

    _logViewState = state;
    ui->outputWidget->setLogViewState(state);
    emit statisticLogStateChanged(state);
}

///
/// \brief FormScriptView::parentGeometry
/// \return
///
QRect FormScriptView::parentGeometry() const
{
    auto wnd = parentWidget();
    return (!wnd->isMaximized() && !wnd->isMinimized()) ? wnd->geometry() : _parentGeometry;
}

///
/// \brief FormScriptView::setParentGeometry
/// \param geometry
///
void FormScriptView::setParentGeometry(const QRect& geometry)
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
/// \brief FormScriptView::show
///
void FormScriptView::show()
{
    QWidget::show();
    connectEditSlots();

    emit showed();
}

///
/// \brief FormScriptView::on_lineEditAddress_valueChanged
///
void FormScriptView::on_lineEditAddress_valueChanged(const QVariant&)
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
/// \brief FormScriptView::on_lineEditLength_valueChanged
///
void FormScriptView::on_lineEditLength_valueChanged(const QVariant&)
{
    emit definitionChanged();
}

///
/// \brief FormScriptView::on_lineEditDeviceId_valueChanged
///
void FormScriptView::on_lineEditDeviceId_valueChanged(const QVariant& oldValue, const QVariant& newValue)
{
    _mbMultiServer.removeDeviceId(oldValue.toInt());
    _mbMultiServer.addDeviceId(newValue.toInt());

    emit definitionChanged();
}

///
/// \brief FormScriptView::on_comboBoxAddressBase_addressBaseChanged
/// \param base
///
void FormScriptView::on_comboBoxAddressBase_addressBaseChanged(AddressBase base)
{
    auto dd = displayDefinition();
    dd.PointAddress = (base == AddressBase::Base1 ? qMax(1, dd.PointAddress + 1) : qMax(0, dd.PointAddress - 1));
    dd.ZeroBasedAddress = (base == AddressBase::Base0);
    setDisplayDefinition(dd);
}

///
/// \brief FormScriptView::on_comboBoxModbusPointType_pointTypeChanged
///
void FormScriptView::on_comboBoxModbusPointType_pointTypeChanged(QModbusDataUnit::RegisterType type)
{
    emit definitionChanged();
    emit pointTypeChanged(type);
}

///
/// \brief FormScriptView::updateStatus
///
void FormScriptView::updateStatus()
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
/// \brief FormScriptView::onDefinitionChanged
///
void FormScriptView::onDefinitionChanged()
{
    updateStatus();

    const auto dd = displayDefinition();
    const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    _mbMultiServer.addUnitMap(formId(), dd.DeviceId, dd.PointType, addr, dd.Length);

    ui->scriptControl->setAddressBase(dd.ZeroBasedAddress ? AddressBase::Base0 : AddressBase::Base1);
    ui->outputWidget->setup(dd, _dataSimulator->simulationMap(), _mbMultiServer.data(dd.DeviceId, dd.PointType, addr, dd.Length));
}

///
/// \brief FormScriptView::scriptControl
/// \return
///
JScriptControl* FormScriptView::scriptControl()
{
    return ui->scriptControl;
}

///
/// \brief FormScriptView::isAutoCompleteEnabled
/// \return
///
bool FormScriptView::isAutoCompleteEnabled() const
{
    return ui->scriptControl->isAutoCompleteEnabled();
}

///
/// \brief FormScriptView::enableAutoComplete
/// \param enable
///
void FormScriptView::enableAutoComplete(bool enable)
{
    ui->scriptControl->enableAutoComplete(enable);
}

///
/// \brief FormScriptView::setScriptFont
/// \param font
///
void FormScriptView::setScriptFont(const QFont& font)
{
    ui->scriptControl->setFont(font);
}

///
/// \brief FormScriptView::on_outputWidget_itemDoubleClicked
/// \param addr
/// \param value
///
void FormScriptView::on_outputWidget_itemDoubleClicked(quint16 addr, const QVariant& value)
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
/// \brief FormScriptView::on_mbConnected
///
void FormScriptView::on_mbConnected(const ConnectionDetails&)
{
    updateStatus();
    ui->outputWidget->clearLogView();

    if(logViewState() == LogViewState::Unknown) {
        setLogViewState(LogViewState::Running);
    }
}

///
/// \brief FormScriptView::on_mbDisconnected
///
void FormScriptView::on_mbDisconnected(const ConnectionDetails&)
{
    updateStatus();
    if(!_mbMultiServer.isConnected()) {
        setLogViewState(LogViewState::Unknown);
    }
}

///
/// \brief FormScriptView::isLoggingRequest
/// \param msgReq
/// \return
///
bool FormScriptView::isLoggingRequest(QSharedPointer<const ModbusMessage> msgReq) const
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
/// \brief FormScriptView::on_mbRequest
/// \param msg
///
void FormScriptView::on_mbRequest(QSharedPointer<const ModbusMessage> msg)
{
    if(_verboseLogging || isLoggingRequest(msg)) {
        ++_requestCount;
        ui->outputWidget->updateTraffic(msg);
    }
}

///
/// \brief FormScriptView::on_mbResponse
/// \param msgReq
/// \param msgResp
///
void FormScriptView::on_mbResponse(QSharedPointer<const ModbusMessage> msgReq, QSharedPointer<const ModbusMessage> msgResp)
{
    if(_verboseLogging || isLoggingRequest(msgReq)) {
        ++_responseCount;
        ui->outputWidget->updateTraffic(msgResp);
    }
}

///
/// \brief FormScriptView::on_mbDataChanged
///
void FormScriptView::on_mbDataChanged(quint8 deviceId, const QModbusDataUnit&)
{
    const auto dd = displayDefinition();
    if(deviceId == dd.DeviceId)
    {
        const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
        ui->outputWidget->updateData(_mbMultiServer.data(deviceId, dd.PointType, addr, dd.Length));
    }
}

///
/// \brief FormScriptView::on_mbDefinitionsChanged
/// \param defs
///
void FormScriptView::on_mbDefinitionsChanged(const ModbusDefinitions& defs)
{
    const auto dd = displayDefinition();
    const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(defs.AddrSpace, dd.ZeroBasedAddress));
    ui->lineEditLength->setInputRange(ModbusLimits::lengthRange(dd.PointAddress, dd.ZeroBasedAddress, defs.AddrSpace));
    ui->outputWidget->setup(dd, _dataSimulator->simulationMap(), _mbMultiServer.data(dd.DeviceId, dd.PointType, addr, dd.Length));
}

///
/// \brief FormScriptView::on_simulationStarted
/// \param mode
/// \param deviceId
/// \param type
/// \param addresses
///
void FormScriptView::on_simulationStarted(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, const QVector<quint16>& addresses)
{
    if(deviceId != ui->lineEditDeviceId->value<quint8>())
        return;

    for(auto&& addr : addresses)
        ui->outputWidget->setSimulated(mode, deviceId, type, addr, true);
}

///
/// \brief FormScriptView::on_simulationStopped
/// \param mode
/// \param deviceId
/// \param type
/// \param addresses
///
void FormScriptView::on_simulationStopped(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, const QVector<quint16>& addresses)
{
    if(deviceId != ui->lineEditDeviceId->value<quint8>())
        return;

    for(auto&& addr : addresses)
        ui->outputWidget->setSimulated(mode, deviceId, type, addr, false);
}

///
/// \brief FormScriptView::on_dataSimulated
/// \param mode
/// \param deviceId
/// \param type
/// \param startAddress
/// \param value
///
void FormScriptView::on_dataSimulated(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, QVariant value)
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
/// \brief FormScriptView::setupScriptBar
///
void FormScriptView::setupScriptBar()
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
    connect(_actionStopScript, &QAction::triggered, this, &FormScriptView::stopScript);

    connect(this, &FormModSim::scriptRunning, this, &FormScriptView::updateScriptBar);
    connect(this, &FormModSim::scriptStopped, this, &FormScriptView::updateScriptBar);
    connect(ui->scriptControl->scriptDocument(), &QTextDocument::contentsChanged,
            this, &FormScriptView::updateScriptBar);

    ui->verticalLayout_4->insertWidget(0, _scriptBar);

    updateScriptBar();
}

///
/// \brief FormScriptView::updateScriptBar
///
void FormScriptView::updateScriptBar()
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
/// \brief FormScriptView::connectEditSlots
///
void FormScriptView::connectEditSlots()
{
    disconnectEditSlots();
    connect(_parent, &MainWindow::selectAll, ui->scriptControl, &JScriptControl::selectAll);
    connect(_parent, &MainWindow::search, ui->scriptControl, &JScriptControl::search);
    connect(_parent, &MainWindow::find, ui->scriptControl, &JScriptControl::showFind);
    connect(_parent, &MainWindow::replace, ui->scriptControl, &JScriptControl::showReplace);
}

///
/// \brief FormScriptView::disconnectEditSlots
///
void FormScriptView::disconnectEditSlots()
{
    disconnect(_parent, &MainWindow::selectAll, ui->scriptControl, &JScriptControl::selectAll);
    disconnect(_parent, &MainWindow::search, ui->scriptControl, &JScriptControl::search);
    disconnect(_parent, &MainWindow::find, ui->scriptControl, &JScriptControl::showFind);
    disconnect(_parent, &MainWindow::replace, ui->scriptControl, &JScriptControl::showReplace);
}





