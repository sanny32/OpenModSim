#include <QPainter>
#include <QPalette>
#include <QDateTime>
#include <QHelpEngine>
#include <QHelpContentWidget>
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

    ui->widgetOutputView->setVisible(false);
    ui->widgetScriptView->setVisible(true);
    ui->scriptControl->setModbusMultiServer(&_mbMultiServer);
    ui->scriptControl->setByteOrder(ui->outputWidget->byteOrder());
    ui->scriptControl->setScriptSource(windowTitle());

    const auto mbDefs = _mbMultiServer.getModbusDefinitions();
    _displayDefinition.FormName = windowTitle();
    _displayDefinition.DeviceId = 1;
    _displayDefinition.PointAddress = 1;
    _displayDefinition.PointType = QModbusDataUnit::HoldingRegisters;
    _displayDefinition.Length = 100;
    _displayDefinition.LogViewLimit = ui->outputWidget->logViewLimit();
    _displayDefinition.AutoscrollLog = ui->outputWidget->autoscrollLogView();
    _displayDefinition.VerboseLogging = _verboseLogging;
    _displayDefinition.HexAddress = false;
    _displayDefinition.HexViewAddress = false;
    _displayDefinition.HexViewDeviceId = false;
    _displayDefinition.HexViewLength = false;
    _displayDefinition.AddrSpace = mbDefs.AddrSpace;
    _displayDefinition.DataViewColumnsDistance = ui->outputWidget->dataViewColumnsDistance();
    _displayDefinition.LeadingZeros = true;
    _displayDefinition.ScriptCfg = _scriptSettings;
    _displayDefinition.normalize();

    server.addDeviceId(_displayDefinition.DeviceId);

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
    const auto deviceId = _displayDefinition.DeviceId;
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
ScriptViewDefinitions FormScriptView::displayDefinition() const
{
    ScriptViewDefinitions dd = _displayDefinition;
    dd.FormName = windowTitle();
    dd.LogViewLimit = ui->outputWidget->logViewLimit();
    dd.AutoscrollLog = ui->outputWidget->autoscrollLogView();
    dd.VerboseLogging = _verboseLogging;
    dd.HexAddress = displayHexAddresses();
    dd.AddrSpace = _mbMultiServer.getModbusDefinitions().AddrSpace;
    dd.DataViewColumnsDistance = ui->outputWidget->dataViewColumnsDistance();
    dd.ScriptCfg = _scriptSettings;
    dd.normalize();
    return dd;
}

///
/// \brief FormScriptView::setDisplayDefinition
/// \param dd
///
void FormScriptView::setDisplayDefinition(const ScriptViewDefinitions& dd)
{
    ScriptViewDefinitions next = dd;
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
    ui->outputWidget->setDataViewColumnsDistance(_displayDefinition.DataViewColumnsDistance);
    ui->outputWidget->setAutosctollLogView(_displayDefinition.AutoscrollLog);

    _verboseLogging = _displayDefinition.VerboseLogging;

    setScriptSettings(_displayDefinition.ScriptCfg);
    setDisplayHexAddresses(_displayDefinition.HexAddress);

    emit definitionChanged();
}

FormDisplayDefinition FormScriptView::displayDefinitionValue() const
{
    return displayDefinition();
}

void FormScriptView::setDisplayDefinitionValue(const FormDisplayDefinition& dd)
{
    if (const auto value = std::get_if<ScriptViewDefinitions>(&dd))
        setDisplayDefinition(*value);
}

///
/// \brief FormScriptView::displayMode
/// \return
///
DisplayMode FormScriptView::displayMode() const
{
    if(ui->widgetScriptView->isVisible())
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
            ui->widgetOutputView->setVisible(false);
            ui->widgetScriptView->setVisible(true);
            ui->scriptControl->setFocus();
        break;

        default:
            ui->widgetScriptView->setVisible(false);
            ui->widgetOutputView->setVisible(true);
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
    _displayDefinition.HexAddress = on;
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
    ui->comboBoxScriptRunMode->setCurrentRunMode(ss.Mode);
    ui->spinBoxScriptInterval->setValue(static_cast<int>(ss.Interval));
    ui->checkBoxScriptRunOnStartup->setChecked(ss.RunOnStartup);
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

    const auto dd = displayDefinition();
    const auto addressBaseText = dd.ZeroBasedAddress ? tr("0-based") : tr("1-based");
    const auto textAddrLen = QString(tr("Address Base: %1\nStart Address: %2\nLength: %3"))
                                 .arg(addressBaseText, QString::number(dd.PointAddress), QString::number(dd.Length));
    auto rcAddrLen = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextWordWrap, textAddrLen);

    const auto textDevIdType = QString(tr("Unit Identifier: %1\nMODBUS Point Type:\n%2"))
                                   .arg(QString::number(dd.DeviceId), enumToString<QModbusDataUnit::RegisterType>(dd.PointType));
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
    const auto deviceId = displayDefinition().DeviceId;
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
    ui->outputWidget->setup(toTrafficViewDefinitions(dd), _dataSimulator->simulationMap(), _mbMultiServer.data(dd.DeviceId, dd.PointType, addr, dd.Length));
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
    const auto deviceId = dd.DeviceId;
    const auto pointType = dd.PointType;
    const auto zeroBasedAddress = dd.ZeroBasedAddress;
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
    _displayDefinition.AddrSpace = defs.AddrSpace;
    _displayDefinition.normalize();
    const auto dd = displayDefinition();
    const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    ui->outputWidget->setup(toTrafficViewDefinitions(dd), _dataSimulator->simulationMap(), _mbMultiServer.data(dd.DeviceId, dd.PointType, addr, dd.Length));
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
    if(deviceId != displayDefinition().DeviceId)
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
    if(deviceId != displayDefinition().DeviceId)
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
    ui->toolBarScript->clear();
    ui->toolBarScript->addWidget(ui->comboBoxScriptRunMode);
    ui->toolBarScript->addWidget(ui->spinBoxScriptInterval);
    ui->toolBarScript->addWidget(ui->checkBoxScriptRunOnStartup);
    ui->toolBarScript->addSeparator();
    ui->toolBarScript->addAction(ui->actionRunScript);
    ui->toolBarScript->addAction(ui->actionStopScript);

    ui->comboBoxScriptRunMode->setCurrentRunMode(_scriptSettings.Mode);
    connect(ui->comboBoxScriptRunMode, &RunModeComboBox::runModeChanged, this, [this](RunMode mode) {
        _scriptSettings.Mode = mode;
    });

    ui->spinBoxScriptInterval->setValue(static_cast<int>(_scriptSettings.Interval));
    connect(ui->spinBoxScriptInterval, &QSpinBox::valueChanged, this, [this](int value) {
        _scriptSettings.Interval = static_cast<uint>(value);
    });

    ui->checkBoxScriptRunOnStartup->setChecked(_scriptSettings.RunOnStartup);
    connect(ui->checkBoxScriptRunOnStartup, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        _scriptSettings.RunOnStartup = (state == Qt::Checked);
    });

    if (auto* runButton = qobject_cast<QToolButton*>(ui->toolBarScript->widgetForAction(ui->actionRunScript)))
        runButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    if (auto* stopButton = qobject_cast<QToolButton*>(ui->toolBarScript->widgetForAction(ui->actionStopScript)))
        stopButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    connect(ui->actionRunScript, &QAction::triggered, this, &FormScriptView::runScript);
    connect(ui->actionStopScript, &QAction::triggered, this, &FormScriptView::stopScript);

    connect(this, &FormModSim::scriptRunning, this, &FormScriptView::updateScriptBar);
    connect(this, &FormModSim::scriptStopped, this, &FormScriptView::updateScriptBar);
    connect(ui->scriptControl->scriptDocument(), &QTextDocument::contentsChanged,
            this, &FormScriptView::updateScriptBar);

    updateScriptBar();
}

///
/// \brief FormScriptView::updateScriptBar
///
void FormScriptView::updateScriptBar()
{
    const bool running = canStopScript();
    ui->actionRunScript->setEnabled(canRunScript());
    ui->actionStopScript->setEnabled(running);
    ui->comboBoxScriptRunMode->setEnabled(!running);
    ui->spinBoxScriptInterval->setEnabled(!running);
    ui->checkBoxScriptRunOnStartup->setEnabled(!running);
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





