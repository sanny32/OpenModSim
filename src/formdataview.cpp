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
#include "formdataview.h"
#include "ui_formdataview.h"

QVersionNumber FormDataView::VERSION = QVersionNumber(1, 15);

///
/// \brief FormDataView::FormDataView
/// \param num
/// \param parent
///
FormDataView::FormDataView(int id, ModbusMultiServer& server, DataSimulator* simulator, MainWindow* parent)
    : FormModSim(parent)
    , ui(new Ui::FormDataView)
    ,_parent(parent)
    ,_formId(id)
    ,_mbMultiServer(server)
    ,_dataSimulator(simulator)
    ,_verboseLogging(true)
{
    Q_ASSERT(parent != nullptr);
    Q_ASSERT(_dataSimulator != nullptr);

    ui->setupUi(this);
    setWindowTitle(QString("Data%1").arg(_formId));
    setWindowIcon(QIcon(":/res/actionShowData.png"));

    ui->lineEditDeviceId->setInputRange(ModbusLimits::slaveRange());
    ui->lineEditDeviceId->setValue(1);
    ui->lineEditDeviceId->setLeadingZeroes(true);
    ui->lineEditDeviceId->setHexButtonVisible(true);
    server.addDeviceId(ui->lineEditDeviceId->value<int>());

    ui->stackedWidget->setCurrentIndex(0);
    ui->outputWidget->enforceDataMode();
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

    connect(this, &FormModSim::definitionChanged, this, &FormDataView::onDefinitionChanged);
    emit definitionChanged();

    ui->outputWidget->setFocus();
    connect(ui->outputWidget, &OutputDataWidget::startTextCaptureError, this, &FormModSim::captureError);
    connect(ui->scriptControl, &JScriptControl::helpContext, this, &FormModSim::helpContextRequested);
    connect(ui->scriptControl, &JScriptControl::scriptStopped, this, &FormModSim::scriptStopped);
    connect(ui->scriptControl, &JScriptControl::consoleMessage, this, &FormModSim::consoleMessage);

    setLogViewState(server.isConnected() ? LogViewState::Running : LogViewState::Unknown);

    connect(&_mbMultiServer, &ModbusMultiServer::request, this, &FormDataView::on_mbRequest);
    connect(&_mbMultiServer, &ModbusMultiServer::response, this, &FormDataView::on_mbResponse);
    connect(&_mbMultiServer, &ModbusMultiServer::connected, this, &FormDataView::on_mbConnected);
    connect(&_mbMultiServer, &ModbusMultiServer::disconnected, this, &FormDataView::on_mbDisconnected);
    connect(&_mbMultiServer, &ModbusMultiServer::dataChanged, this, &FormDataView::on_mbDataChanged);
    connect(&_mbMultiServer, &ModbusMultiServer::definitionsChanged, this, &FormDataView::on_mbDefinitionsChanged);

    connect(_dataSimulator, &DataSimulator::simulationStarted, this, &FormDataView::on_simulationStarted);
    connect(_dataSimulator, &DataSimulator::simulationStopped, this, &FormDataView::on_simulationStopped);
    connect(_dataSimulator, &DataSimulator::dataSimulated, this, &FormDataView::on_dataSimulated);

    setupDisplayBar();
    setupScriptBar();
}

///
/// \brief FormDataView::~FormDataView
///
FormDataView::~FormDataView()
{
    delete ui;
}

void FormDataView::saveSettings(QSettings& out) const
{
    out << const_cast<FormDataView*>(this);
}

void FormDataView::loadSettings(QSettings& in)
{
    in >> this;
}

void FormDataView::saveXml(QXmlStreamWriter& xml) const
{
    xml << const_cast<FormDataView*>(this);
}

void FormDataView::loadXml(QXmlStreamReader& xml)
{
    xml >> this;
}

///
/// \brief FormDataView::changeEvent
/// \param e
///
void FormDataView::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        updateStatus();
    }

    QWidget::changeEvent(e);
}

///
/// \brief FormDataView::closeEvent
/// \param event
///
void FormDataView::closeEvent(QCloseEvent* event)
{
    const auto deviceId = ui->lineEditDeviceId->value<quint8>();
    _mbMultiServer.removeDeviceId(deviceId);
    _mbMultiServer.removeUnitMap(formId(), deviceId);

    emit closing();
    QWidget::closeEvent(event);
}

///
/// \brief FormDataView::mouseDoubleClickEvent
/// \param event
/// \return
///
void FormDataView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(ui->frameDataDefinition->geometry().contains(event->pos())) {
        emit doubleClicked();
    }

    return QWidget::mouseDoubleClickEvent(event);
}

///
/// \brief FormDataView::filename
/// \return
///
QString FormDataView::filename() const
{
    return _filename;
}

///
/// \brief FormDataView::setFilename
/// \param filename
///
void FormDataView::setFilename(const QString& filename)
{
    _filename = filename;
}

///
/// \brief FormDataView::data
/// \return
///
QVector<quint16> FormDataView::data() const
{
    return ui->outputWidget->data();
}

///
/// \brief FormDataView::displayDefinition
/// \return
///
DisplayDefinition FormDataView::displayDefinition() const
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
/// \brief FormDataView::setDisplayDefinition
/// \param dd
///
void FormDataView::setDisplayDefinition(const DisplayDefinition& dd)
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
/// \brief FormDataView::displayMode
/// \return
///
DisplayMode FormDataView::displayMode() const
{
    if(ui->stackedWidget->currentIndex() == 1)
        return DisplayMode::Script;
    else
        return ui->outputWidget->displayMode();
}

///
/// \brief FormDataView::setDisplayMode
/// \param mode
///
void FormDataView::setDisplayMode(DisplayMode mode)
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
/// \brief FormDataView::dataDisplayMode
/// \return
///
DataDisplayMode FormDataView::dataDisplayMode() const
{
    return ui->outputWidget->dataDisplayMode();
}

///
/// \brief FormDataView::displayHexAddresses
/// \return
///
bool FormDataView::displayHexAddresses() const
{
    return ui->outputWidget->displayHexAddresses();
}

///
/// \brief FormDataView::setDisplayHexAddresses
/// \param on
///
void FormDataView::setDisplayHexAddresses(bool on)
{
    ui->outputWidget->setDisplayHexAddresses(on);

    const auto defs = _mbMultiServer.getModbusDefinitions();
    ui->lineEditAddress->setInputMode(on ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(defs.AddrSpace, ui->comboBoxAddressBase->currentAddressBase() == AddressBase::Base0));
}

///
/// \brief FormDataView::captureMode
///
CaptureMode FormDataView::captureMode() const
{
    return ui->outputWidget->captureMode();
}

///
/// \brief FormDataView::startTextCapture
/// \param file
///
void FormDataView::startTextCapture(const QString& file)
{
    ui->outputWidget->startTextCapture(file);
}

///
/// \brief FormDataView::stopTextCapture
///
void FormDataView::stopTextCapture()
{
    ui->outputWidget->stopTextCapture();
}

///
/// \brief FormDataView::setDataDisplayMode
/// \param mode
///
void FormDataView::setDataDisplayMode(DataDisplayMode mode)
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
/// \brief FormDataView::scriptSettings
/// \return
///
ScriptSettings FormDataView::scriptSettings() const
{
    return _scriptSettings;
}

///
/// \brief FormDataView::setScriptSettings
/// \param ss
///
void FormDataView::setScriptSettings(const ScriptSettings& ss)
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
/// \brief FormDataView::byteOrder
/// \return
///
ByteOrder FormDataView::byteOrder() const
{
    return *ui->outputWidget->byteOrder();
}

///
/// \brief FormDataView::setByteOrder
/// \param order
///
void FormDataView::setByteOrder(ByteOrder order)
{
    ui->outputWidget->setByteOrder(order);
    emit byteOrderChanged(order);
    updateDisplayBar();
}

///
/// \brief FormDataView::codepage
/// \return
///
QString FormDataView::codepage() const
{
    return ui->outputWidget->codepage();
}

///
/// \brief FormDataView::setCodepage
/// \param name
///
void FormDataView::setCodepage(const QString& name)
{
    ui->outputWidget->setCodepage(name);
    emit codepageChanged(name);
    if (_ansiMenu) _ansiMenu->selectCodepage(name);
}

///
/// \brief FormDataView::backgroundColor
/// \return
///
QColor FormDataView::backgroundColor() const
{
    return ui->outputWidget->backgroundColor();
}

///
/// \brief FormDataView::setBackgroundColor
/// \param clr
///
void FormDataView::setBackgroundColor(const QColor& clr)
{
    ui->outputWidget->setBackgroundColor(clr);
}

///
/// \brief FormDataView::foregroundColor
/// \return
///
QColor FormDataView::foregroundColor() const
{
    return ui->outputWidget->foregroundColor();
}

///
/// \brief FormDataView::setForegroundColor
/// \param clr
///
void FormDataView::setForegroundColor(const QColor& clr)
{
    ui->outputWidget->setForegroundColor(clr);
}

///
/// \brief FormDataView::statusColor
/// \return
///
QColor FormDataView::statusColor() const
{
    return ui->outputWidget->statusColor();
}

///
/// \brief FormDataView::setStatusColor
/// \param clr
///
void FormDataView::setStatusColor(const QColor& clr)
{
    ui->outputWidget->setStatusColor(clr);
}

///
/// \brief FormDataView::font
/// \return
///
QFont FormDataView::font() const
{
   return ui->outputWidget->font();
}

///
/// \brief FormDataView::setFont
/// \param font
///
void FormDataView::setFont(const QFont& font)
{
    ui->outputWidget->setFont(font);
}

///
/// \brief FormDataView::zoomPercent
/// \return
///
int FormDataView::zoomPercent() const
{
    return ui->outputWidget->zoomPercent();
}

///
/// \brief FormDataView::setZoomPercent
/// \param zoomPercent
///
void FormDataView::setZoomPercent(int zoomPercent)
{
    ui->outputWidget->setZoomPercent(zoomPercent);
}

///
/// \brief FormDataView::print
/// \param printer
///
void FormDataView::print(QPrinter* printer)
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
/// \brief FormDataView::simulationMap
/// \return
///
ModbusSimulationMap2 FormDataView::simulationMap() const
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
/// \brief FormDataView::serializeModbusDataUnit
/// \param deviceId
/// \param type
/// \param startAddress
/// \param length
/// \return
///
QModbusDataUnit FormDataView::serializeModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, quint16 length) const
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
/// \brief FormDataView::startSimulation
/// \param type
/// \param addr
/// \param params
///
void FormDataView::startSimulation(QModbusDataUnit::RegisterType type, quint16 addr, const ModbusSimulationParams& params)
{
    const auto deviceId = ui->lineEditDeviceId->value<quint8>();
    _dataSimulator->startSimulation(deviceId, type, addr, params);
}

///
/// \brief FormDataView::configureModbusDataUnit
/// \param deviceId
/// \param type
/// \param startAddress
/// \param values
///
void FormDataView::configureModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, const QVector<quint16>& values) const
{
    QModbusDataUnit unit;
    unit.setRegisterType(type);
    unit.setStartAddress(startAddress);
    unit.setValues(values);
    _mbMultiServer.setData(deviceId, unit);
}


///
/// \brief FormDataView::descriptionMap
/// \return
///
AddressDescriptionMap2 FormDataView::descriptionMap() const
{
    return ui->outputWidget->descriptionMap();
}

///
/// \brief FormDataView::setDescription
/// \param deviceId
/// \param type
/// \param addr
/// \param desc
///
void FormDataView::setDescription(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc)
{
    ui->outputWidget->setDescription(deviceId, type, addr, desc);
}

///
/// \brief FormDataView::colorMap
/// \return
///
AddressColorMap FormDataView::colorMap() const
{
    return ui->outputWidget->colorMap();
}

///
/// \brief FormDataView::setColor
/// \param deviceId
/// \param type
/// \param addr
/// \param clr
///
void FormDataView::setColor(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QColor& clr)
{
    ui->outputWidget->setColor(deviceId, type, addr, clr);
}

///
/// \brief FormDataView::resetCtrs
///
void FormDataView::resetCtrs()
{
    _requestCount = 0;
    _responseCount = 0;
    ui->outputWidget->clearLogView();
    emit statisticCtrsReseted();
}

///
/// \brief FormDataView::requestCount
/// \return
///
uint FormDataView::requestCount() const
{
    return _requestCount;
}

///
/// \brief FormDataView::responseCount
/// \return
///
uint FormDataView::responseCount() const
{
    return _responseCount;
}

///
/// \brief FormDataView::setStatisticCounters
/// \param requests
/// \param responses
///
void FormDataView::setStatisticCounters(uint requests, uint responses)
{
    _requestCount = requests;
    _responseCount = responses;
}

///
/// \brief FormDataView::script
/// \return
///
QString FormDataView::script() const
{
    return ui->scriptControl->script();
}

///
/// \brief FormDataView::setScript
/// \param text
///
void FormDataView::setScript(const QString& text)
{
    ui->scriptControl->setScript(text);
}

///
/// \brief FormDataView::scriptDocument
/// \return
///
QTextDocument* FormDataView::scriptDocument() const
{
    return ui->scriptControl->scriptDocument();
}

///
/// \brief FormDataView::setScriptDocument
/// \param document
///
void FormDataView::setScriptDocument(QTextDocument* document)
{
    ui->scriptControl->setScriptDocument(document);
}

int FormDataView::scriptCursorPosition() const
{
    return ui->scriptControl->cursorPosition();
}

void FormDataView::setScriptCursorPosition(int pos)
{
    ui->scriptControl->setCursorPosition(pos);
}

int FormDataView::scriptScrollPosition() const
{
    return ui->scriptControl->scrollPosition();
}

void FormDataView::setScriptScrollPosition(int pos)
{
    ui->scriptControl->setScrollPosition(pos);
}

///
/// \brief FormDataView::searchText
/// \return
///
QString FormDataView::searchText() const
{
    return ui->scriptControl->searchText();
}

///
/// \brief FormDataView::canRunScript
/// \return
///
bool FormDataView::canRunScript() const
{
    return !ui->scriptControl->script().isEmpty() &&
           !ui->scriptControl->isRunning();
}

///
/// \brief FormDataView::canStopScript
/// \return
///
bool FormDataView::canStopScript() const
{
    return ui->scriptControl->isRunning();
}

///
/// \brief FormDataView::canUndo
/// \return
///
bool FormDataView::canUndo() const
{
    return ui->scriptControl->canUndo();
}

///
/// \brief FormDataView::canRedo
/// \return
///
bool FormDataView::canRedo() const
{
    return ui->scriptControl->canRedo();
}

///
/// \brief FormDataView::canPaste
/// \return
///
bool FormDataView::canPaste() const
{
    return ui->scriptControl->canPaste();
}

///
/// \brief FormDataView::runScript
/// \param interval
///
void FormDataView::runScript()
{
    emit scriptRunning();
    ui->scriptControl->runScript(_scriptSettings.Mode, _scriptSettings.Interval);
}

///
/// \brief FormDataView::stopScript
///
void FormDataView::stopScript()
{
    ui->scriptControl->stopScript();
}

///
/// \brief FormDataView::logViewState
/// \return
///
LogViewState FormDataView::logViewState() const
{
    return _logViewState;
}

///
/// \brief FormDataView::setLogViewState
/// \param state
///
void FormDataView::setLogViewState(LogViewState state)
{
    if(_logViewState == state)
        return;

    _logViewState = state;
    ui->outputWidget->setLogViewState(state);
    emit statisticLogStateChanged(state);
}

///
/// \brief FormDataView::parentGeometry
/// \return
///
QRect FormDataView::parentGeometry() const
{
    auto wnd = parentWidget();
    return (!wnd->isMaximized() && !wnd->isMinimized()) ? wnd->geometry() : _parentGeometry;
}

///
/// \brief FormDataView::setParentGeometry
/// \param geometry
///
void FormDataView::setParentGeometry(const QRect& geometry)
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
/// \brief FormDataView::show
///
void FormDataView::show()
{
    QWidget::show();
    connectEditSlots();

    emit showed();
}

///
/// \brief FormDataView::on_lineEditAddress_valueChanged
///
void FormDataView::on_lineEditAddress_valueChanged(const QVariant&)
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
/// \brief FormDataView::on_lineEditLength_valueChanged
///
void FormDataView::on_lineEditLength_valueChanged(const QVariant&)
{
    emit definitionChanged();
}

///
/// \brief FormDataView::on_lineEditDeviceId_valueChanged
///
void FormDataView::on_lineEditDeviceId_valueChanged(const QVariant& oldValue, const QVariant& newValue)
{
    _mbMultiServer.removeDeviceId(oldValue.toInt());
    _mbMultiServer.addDeviceId(newValue.toInt());

    emit definitionChanged();
}

///
/// \brief FormDataView::on_comboBoxAddressBase_addressBaseChanged
/// \param base
///
void FormDataView::on_comboBoxAddressBase_addressBaseChanged(AddressBase base)
{
    auto dd = displayDefinition();
    dd.PointAddress = (base == AddressBase::Base1 ? qMax(1, dd.PointAddress + 1) : qMax(0, dd.PointAddress - 1));
    dd.ZeroBasedAddress = (base == AddressBase::Base0);
    setDisplayDefinition(dd);
}

///
/// \brief FormDataView::on_comboBoxModbusPointType_pointTypeChanged
///
void FormDataView::on_comboBoxModbusPointType_pointTypeChanged(QModbusDataUnit::RegisterType type)
{
    emit definitionChanged();
    emit pointTypeChanged(type);
    updateDisplayBar();
}

///
/// \brief FormDataView::updateStatus
///
void FormDataView::updateStatus()
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
/// \brief FormDataView::onDefinitionChanged
///
void FormDataView::onDefinitionChanged()
{
    updateStatus();

    const auto dd = displayDefinition();
    const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    _mbMultiServer.addUnitMap(formId(), dd.DeviceId, dd.PointType, addr, dd.Length);

    ui->scriptControl->setAddressBase(dd.ZeroBasedAddress ? AddressBase::Base0 : AddressBase::Base1);
    ui->outputWidget->setup(dd, _dataSimulator->simulationMap(), _mbMultiServer.data(dd.DeviceId, dd.PointType, addr, dd.Length));
}

///
/// \brief FormDataView::scriptControl
/// \return
///
JScriptControl* FormDataView::scriptControl()
{
    return ui->scriptControl;
}

///
/// \brief FormDataView::isAutoCompleteEnabled
/// \return
///
bool FormDataView::isAutoCompleteEnabled() const
{
    return ui->scriptControl->isAutoCompleteEnabled();
}

///
/// \brief FormDataView::enableAutoComplete
/// \param enable
///
void FormDataView::enableAutoComplete(bool enable)
{
    ui->scriptControl->enableAutoComplete(enable);
}

///
/// \brief FormDataView::setScriptFont
/// \param font
///
void FormDataView::setScriptFont(const QFont& font)
{
    ui->scriptControl->setFont(font);
}

///
/// \brief FormDataView::on_outputWidget_itemDoubleClicked
/// \param addr
/// \param value
///
void FormDataView::on_outputWidget_itemDoubleClicked(quint16 addr, const QVariant& value)
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
/// \brief FormDataView::on_mbConnected
///
void FormDataView::on_mbConnected(const ConnectionDetails&)
{
    updateStatus();
    ui->outputWidget->clearLogView();

    if(logViewState() == LogViewState::Unknown) {
        setLogViewState(LogViewState::Running);
    }
}

///
/// \brief FormDataView::on_mbDisconnected
///
void FormDataView::on_mbDisconnected(const ConnectionDetails&)
{
    updateStatus();
    if(!_mbMultiServer.isConnected()) {
        setLogViewState(LogViewState::Unknown);
    }
}

///
/// \brief FormDataView::isLoggingRequest
/// \param msgReq
/// \return
///
bool FormDataView::isLoggingRequest(QSharedPointer<const ModbusMessage> msgReq) const
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
/// \brief FormDataView::on_mbRequest
/// \param msg
///
void FormDataView::on_mbRequest(QSharedPointer<const ModbusMessage> msg)
{
    if(_verboseLogging || isLoggingRequest(msg)) {
        ++_requestCount;
        ui->outputWidget->updateTraffic(msg);
    }
}

///
/// \brief FormDataView::on_mbResponse
/// \param msgReq
/// \param msgResp
///
void FormDataView::on_mbResponse(QSharedPointer<const ModbusMessage> msgReq, QSharedPointer<const ModbusMessage> msgResp)
{
    if(_verboseLogging || isLoggingRequest(msgReq)) {
        ++_responseCount;
        ui->outputWidget->updateTraffic(msgResp);
    }
}

///
/// \brief FormDataView::on_mbDataChanged
///
void FormDataView::on_mbDataChanged(quint8 deviceId, const QModbusDataUnit&)
{
    const auto dd = displayDefinition();
    if(deviceId == dd.DeviceId)
    {
        const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
        ui->outputWidget->updateData(_mbMultiServer.data(deviceId, dd.PointType, addr, dd.Length));
    }
}

///
/// \brief FormDataView::on_mbDefinitionsChanged
/// \param defs
///
void FormDataView::on_mbDefinitionsChanged(const ModbusDefinitions& defs)
{
    const auto dd = displayDefinition();
    const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(defs.AddrSpace, dd.ZeroBasedAddress));
    ui->lineEditLength->setInputRange(ModbusLimits::lengthRange(dd.PointAddress, dd.ZeroBasedAddress, defs.AddrSpace));
    ui->outputWidget->setup(dd, _dataSimulator->simulationMap(), _mbMultiServer.data(dd.DeviceId, dd.PointType, addr, dd.Length));
}

///
/// \brief FormDataView::on_simulationStarted
/// \param mode
/// \param deviceId
/// \param type
/// \param addresses
///
void FormDataView::on_simulationStarted(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, const QVector<quint16>& addresses)
{
    if(deviceId != ui->lineEditDeviceId->value<quint8>())
        return;

    for(auto&& addr : addresses)
        ui->outputWidget->setSimulated(mode, deviceId, type, addr, true);
}

///
/// \brief FormDataView::on_simulationStopped
/// \param mode
/// \param deviceId
/// \param type
/// \param addresses
///
void FormDataView::on_simulationStopped(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, const QVector<quint16>& addresses)
{
    if(deviceId != ui->lineEditDeviceId->value<quint8>())
        return;

    for(auto&& addr : addresses)
        ui->outputWidget->setSimulated(mode, deviceId, type, addr, false);
}

///
/// \brief FormDataView::on_dataSimulated
/// \param mode
/// \param deviceId
/// \param type
/// \param startAddress
/// \param value
///
void FormDataView::on_dataSimulated(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, QVariant value)
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
/// \brief FormDataView::setupScriptBar
///
void FormDataView::setupScriptBar()
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
    connect(_actionStopScript, &QAction::triggered, this, &FormDataView::stopScript);

    connect(this, &FormModSim::scriptRunning, this, &FormDataView::updateScriptBar);
    connect(this, &FormModSim::scriptStopped, this, &FormDataView::updateScriptBar);
    connect(ui->scriptControl->scriptDocument(), &QTextDocument::contentsChanged,
            this, &FormDataView::updateScriptBar);

    ui->verticalLayout_4->insertWidget(0, _scriptBar);

    updateScriptBar();
}

///
/// \brief FormDataView::updateScriptBar
///
void FormDataView::updateScriptBar()
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
/// \brief FormDataView::setupDisplayBar
///
void FormDataView::setupDisplayBar()
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
    connect(_ansiMenu, &AnsiMenu::codepageSelected, this, &FormDataView::setCodepage);
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
/// \brief FormDataView::updateDisplayBar
///
void FormDataView::updateDisplayBar()
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
/// \brief FormDataView::connectEditSlots
///
void FormDataView::connectEditSlots()
{
    disconnectEditSlots();
    connect(_parent, &MainWindow::selectAll, ui->scriptControl, &JScriptControl::selectAll);
    connect(_parent, &MainWindow::search, ui->scriptControl, &JScriptControl::search);
    connect(_parent, &MainWindow::find, ui->scriptControl, &JScriptControl::showFind);
    connect(_parent, &MainWindow::replace, ui->scriptControl, &JScriptControl::showReplace);
}

///
/// \brief FormDataView::disconnectEditSlots
///
void FormDataView::disconnectEditSlots()
{
    disconnect(_parent, &MainWindow::selectAll, ui->scriptControl, &JScriptControl::selectAll);
    disconnect(_parent, &MainWindow::search, ui->scriptControl, &JScriptControl::search);
    disconnect(_parent, &MainWindow::find, ui->scriptControl, &JScriptControl::showFind);
    disconnect(_parent, &MainWindow::replace, ui->scriptControl, &JScriptControl::showReplace);
}




