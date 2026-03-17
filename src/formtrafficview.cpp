#include <QPainter>
#include <QPalette>
#include <QDateTime>
#include <QHelpEngine>
#include <QHelpContentWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QToolButton>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QSplitter>
#include "modbuslimits.h"
#include "mainwindow.h"
#include "modbusmessages.h"
#include "datasimulator.h"
#include "dialogwritestatusregister.h"
#include "dialogwriteregister.h"
#include "controls/numericlineedit.h"
#include "controls/addressbasecombobox.h"
#include "controls/pointtypecombobox.h"
#include "controls/statisticwidget.h"
#include "controls/runmodecombobox.h"
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

    _widgetOutputView = new QWidget(this);
    _widgetOutputView->setObjectName(QStringLiteral("widgetOutputView"));
    auto* outputViewLayout = new QVBoxLayout(_widgetOutputView);
    outputViewLayout->setContentsMargins(0, 0, 0, 0);
    outputViewLayout->setSpacing(0);

    _toolBarTrafficFilter = new QToolBar(_widgetOutputView);
    _toolBarTrafficFilter->setObjectName(QStringLiteral("toolBarTrafficFilter"));
    _toolBarTrafficFilter->setIconSize(QSize(16, 16));
    outputViewLayout->addWidget(_toolBarTrafficFilter);

    _splitter = new QSplitter(Qt::Vertical, _widgetOutputView);
    _splitter->setObjectName(QStringLiteral("splitter"));
    _splitter->setChildrenCollapsible(false);

    _frameDataDefinition = new QFrame(_splitter);
    _frameDataDefinition->setObjectName(QStringLiteral("frameDataDefinition"));
    _frameDataDefinition->setAutoFillBackground(true);
    _frameDataDefinition->setFrameShape(QFrame::WinPanel);
    _frameDataDefinition->setFrameShadow(QFrame::Sunken);
    auto* definitionLayout = new QHBoxLayout(_frameDataDefinition);
    definitionLayout->setContentsMargins(9, 9, 9, 9);
    definitionLayout->setSpacing(9);

    _comboBoxAddressBase = new AddressBaseComboBox(_frameDataDefinition);
    _comboBoxAddressBase->setObjectName(QStringLiteral("comboBoxAddressBase"));
    _comboBoxAddressBase->setMinimumHeight(26);
    definitionLayout->addWidget(_comboBoxAddressBase);

    _lineEditAddress = new NumericLineEdit(_frameDataDefinition);
    _lineEditAddress->setObjectName(QStringLiteral("lineEditAddress"));
    _lineEditAddress->setMinimumHeight(26);
    _lineEditAddress->setContextMenuPolicy(Qt::NoContextMenu);
    _lineEditAddress->setAcceptDrops(false);
    definitionLayout->addWidget(_lineEditAddress);

    _lineEditLength = new NumericLineEdit(_frameDataDefinition);
    _lineEditLength->setObjectName(QStringLiteral("lineEditLength"));
    _lineEditLength->setMinimumHeight(26);
    _lineEditLength->setContextMenuPolicy(Qt::NoContextMenu);
    _lineEditLength->setAcceptDrops(false);
    definitionLayout->addWidget(_lineEditLength);

    _lineEditDeviceId = new NumericLineEdit(_frameDataDefinition);
    _lineEditDeviceId->setObjectName(QStringLiteral("lineEditDeviceId"));
    _lineEditDeviceId->setMinimumHeight(26);
    _lineEditDeviceId->setMaximumWidth(65);
    _lineEditDeviceId->setContextMenuPolicy(Qt::NoContextMenu);
    _lineEditDeviceId->setAcceptDrops(false);
    definitionLayout->addWidget(_lineEditDeviceId);

    _comboBoxModbusPointType = new PointTypeComboBox(_frameDataDefinition);
    _comboBoxModbusPointType->setObjectName(QStringLiteral("comboBoxModbusPointType"));
    _comboBoxModbusPointType->setMinimumHeight(26);
    _comboBoxModbusPointType->setFocusPolicy(Qt::StrongFocus);
    _comboBoxModbusPointType->setFrame(true);
    definitionLayout->addWidget(_comboBoxModbusPointType);

    _statisticWidget = new StatisticWidget(_frameDataDefinition);
    _statisticWidget->setObjectName(QStringLiteral("statisticWidget"));
    _statisticWidget->setMinimumWidth(190);
    definitionLayout->addStretch(1);
    definitionLayout->addWidget(_statisticWidget);

    auto* frameOutput = new QFrame(_splitter);
    frameOutput->setObjectName(QStringLiteral("frameOutput"));
    frameOutput->setFrameShape(QFrame::WinPanel);
    frameOutput->setFrameShadow(QFrame::Sunken);
    auto* frameOutputLayout = new QVBoxLayout(frameOutput);
    frameOutputLayout->setContentsMargins(0, 0, 0, 0);
    frameOutputLayout->setSpacing(0);

    _outputWidget = new OutputTrafficWidget(frameOutput);
    _outputWidget->setObjectName(QStringLiteral("outputWidget"));
    _outputWidget->setAutoFillBackground(true);
    frameOutputLayout->addWidget(_outputWidget);

    outputViewLayout->addWidget(_splitter);
    ui->verticalLayout_2->insertWidget(0, _widgetOutputView);

    connect(_lineEditAddress, qOverload<const QVariant&>(&NumericLineEdit::valueChanged), this, &FormTrafficView::on_lineEditAddress_valueChanged);
    connect(_lineEditLength, qOverload<const QVariant&>(&NumericLineEdit::valueChanged), this, &FormTrafficView::on_lineEditLength_valueChanged);
    connect(_lineEditDeviceId, qOverload<const QVariant&, const QVariant&>(&NumericLineEdit::valueChanged), this, &FormTrafficView::on_lineEditDeviceId_valueChanged);
    connect(_comboBoxAddressBase, &AddressBaseComboBox::addressBaseChanged, this, &FormTrafficView::on_comboBoxAddressBase_addressBaseChanged);
    connect(_comboBoxModbusPointType, &PointTypeComboBox::pointTypeChanged, this, &FormTrafficView::on_comboBoxModbusPointType_pointTypeChanged);
    connect(_outputWidget, &OutputTrafficWidget::itemDoubleClicked, this, &FormTrafficView::on_outputWidget_itemDoubleClicked);

    setWindowTitle(QString("Traffic%1").arg(_formId));
    setWindowIcon(QIcon(":/res/actionShowTraffic.png"));
    setupTrafficFilterBar();

    _frameDataDefinition->hide();
    _frameDataDefinition->setMaximumHeight(0);
    _splitter->setSizes({0, 1});

    _lineEditDeviceId->setInputRange(ModbusLimits::slaveRange());
    _lineEditDeviceId->setValue(1);
    _lineEditDeviceId->setLeadingZeroes(true);
    _lineEditDeviceId->setHexButtonVisible(true);
    server.addDeviceId(_lineEditDeviceId->value<int>());

    _widgetOutputView->setVisible(true);
    ui->toolBarScript->setVisible(false);
    ui->scriptControl->setVisible(false);
    _outputWidget->enforceTrafficMode();
    ui->scriptControl->setModbusMultiServer(&_mbMultiServer);
    ui->scriptControl->setByteOrder(_outputWidget->byteOrder());
    ui->scriptControl->setScriptSource(windowTitle());

    const auto mbDefs = _mbMultiServer.getModbusDefinitions();

    _lineEditAddress->setLeadingZeroes(true);
    _lineEditAddress->setInputRange(ModbusLimits::addressRange(mbDefs.AddrSpace, true));
    _lineEditAddress->setValue(0);
    _lineEditAddress->setHexButtonVisible(true);

    _lineEditLength->setInputRange(ModbusLimits::lengthRange(0, true, mbDefs.AddrSpace));
    _lineEditLength->setValue(100);
    _lineEditLength->setHexButtonVisible(true);

    _comboBoxAddressBase->setCurrentAddressBase(AddressBase::Base1);
    _comboBoxModbusPointType->setCurrentPointType(QModbusDataUnit::HoldingRegisters);

    connect(this, &FormModSim::definitionChanged, this, &FormTrafficView::onDefinitionChanged);
    emit definitionChanged();

    _outputWidget->setFocus();
    connect(_outputWidget, &OutputTrafficWidget::startTextCaptureError, this, &FormModSim::captureError);
    connect(ui->scriptControl, &JScriptControl::helpContext, this, &FormModSim::helpContextRequested);
    connect(ui->scriptControl, &JScriptControl::scriptStopped, this, &FormModSim::scriptStopped);
    connect(ui->scriptControl, &JScriptControl::consoleMessage, this, &FormModSim::consoleMessage);

    setLogViewState(server.isConnected() ? LogViewState::Running : LogViewState::Unknown);
    connect(_statisticWidget, &StatisticWidget::ctrsReseted, _outputWidget, &OutputTrafficWidget::clearLogView);
    connect(_statisticWidget, &StatisticWidget::logStateChanged, _outputWidget, &OutputTrafficWidget::setLogViewState);
    connect(_statisticWidget, &StatisticWidget::ctrsReseted, this, &FormModSim::statisticCtrsReseted);
    connect(_statisticWidget, &StatisticWidget::logStateChanged, this, &FormModSim::statisticLogStateChanged);

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
        if (_scriptIntervalSpin)
            _scriptIntervalSpin->setSuffix(tr(" ms"));
        if (_scriptRunOnStartupCheck)
            _scriptRunOnStartupCheck->setText(tr("Run on startup"));
        retranslateTrafficFilterBar();
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
    const auto deviceId = _lineEditDeviceId->value<quint8>();
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
    if(_frameDataDefinition->geometry().contains(event->pos())) {
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
    return _outputWidget->data();
}

///
/// \brief FormTrafficView::displayDefinition
/// \return
///
TrafficViewDefinitions FormTrafficView::displayDefinition() const
{
    TrafficViewDefinitions dd;
    dd.FormName = windowTitle();
    dd.DeviceId = _lineEditDeviceId->value<int>();
    dd.PointAddress = _lineEditAddress->value<int>();
    dd.PointType = _comboBoxModbusPointType->currentPointType();
    dd.Length = _lineEditLength->value<int>();
    dd.ZeroBasedAddress = _lineEditAddress->range<int>().from() == 0;
    dd.LogViewLimit = _outputWidget->logViewLimit();
    dd.AutoscrollLog = _outputWidget->autoscrollLogView();
    dd.VerboseLogging = _verboseLogging;
    dd.HexAddress = displayHexAddresses();
    dd.HexViewAddress  = _lineEditAddress->hexView();
    dd.HexViewDeviceId = _lineEditDeviceId->hexView();
    dd.HexViewLength   = _lineEditLength->hexView();
    dd.AddrSpace = _mbMultiServer.getModbusDefinitions().AddrSpace;
    dd.DataViewColumnsDistance = _outputWidget->dataViewColumnsDistance();
    dd.LeadingZeros = _lineEditDeviceId->leadingZeroes();
    dd.ScriptCfg = _scriptSettings;

    return dd;
}

///
/// \brief FormTrafficView::setDisplayDefinition
/// \param dd
///
void FormTrafficView::setDisplayDefinition(const TrafficViewDefinitions& dd)
{
    if(!dd.FormName.isEmpty())
        setWindowTitle(dd.FormName);

    const auto defs = _mbMultiServer.getModbusDefinitions();

    _lineEditDeviceId->setLeadingZeroes(dd.LeadingZeros);
    _lineEditDeviceId->setValue(dd.DeviceId);

    _comboBoxAddressBase->blockSignals(true);
    _comboBoxAddressBase->setCurrentAddressBase(dd.ZeroBasedAddress ? AddressBase::Base0 : AddressBase::Base1);
    _comboBoxAddressBase->blockSignals(false);

    _lineEditAddress->blockSignals(true);
    _lineEditAddress->setLeadingZeroes(dd.LeadingZeros);
    _lineEditAddress->setInputRange(ModbusLimits::addressRange(defs.AddrSpace, dd.ZeroBasedAddress));
    _lineEditAddress->setValue(dd.PointAddress);
    _lineEditAddress->blockSignals(false);

    _lineEditLength->blockSignals(true);
    _lineEditLength->setLeadingZeroes(dd.LeadingZeros);
    _lineEditLength->setInputRange(ModbusLimits::lengthRange(dd.PointAddress, dd.ZeroBasedAddress, defs.AddrSpace));
    _lineEditLength->setValue(dd.Length);
    _lineEditLength->blockSignals(false);

    _comboBoxModbusPointType->blockSignals(true);
    _comboBoxModbusPointType->setCurrentPointType(dd.PointType);
    _comboBoxModbusPointType->blockSignals(false);

    _outputWidget->setLogViewLimit(dd.LogViewLimit);
    _outputWidget->setDataViewColumnsDistance(dd.DataViewColumnsDistance);
    _outputWidget->setAutosctollLogView(dd.AutoscrollLog);
    if (_rowLimitCombo) {
        int idx = _rowLimitCombo->findData(dd.LogViewLimit);
        if (idx < 0) {
            _rowLimitCombo->addItem(QString::number(dd.LogViewLimit), dd.LogViewLimit);
            idx = _rowLimitCombo->findData(dd.LogViewLimit);
        }
        if (idx >= 0)
            _rowLimitCombo->setCurrentIndex(idx);
    }

    _verboseLogging = dd.VerboseLogging;

    setScriptSettings(dd.ScriptCfg);
    setDisplayHexAddresses(dd.HexAddress);

    _lineEditDeviceId->setHexView(dd.HexViewDeviceId);
    _lineEditAddress->setHexView(dd.HexViewAddress);
    _lineEditLength->setHexView(dd.HexViewLength);

    emit definitionChanged();
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
/// \brief FormTrafficView::displayMode
/// \return
///
DisplayMode FormTrafficView::displayMode() const
{
    if(ui->scriptControl->isVisible())
        return DisplayMode::Script;

        return _outputWidget->displayMode();
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
            _widgetOutputView->setVisible(false);
            ui->toolBarScript->setVisible(true);
            ui->scriptControl->setVisible(true);
            ui->scriptControl->setFocus();
        break;

        default:
            ui->toolBarScript->setVisible(false);
            ui->scriptControl->setVisible(false);
            _widgetOutputView->setVisible(true);
            _outputWidget->setDisplayMode(mode);
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
    return _outputWidget->dataDisplayMode();
}

///
/// \brief FormTrafficView::displayHexAddresses
/// \return
///
bool FormTrafficView::displayHexAddresses() const
{
    return _outputWidget->displayHexAddresses();
}

///
/// \brief FormTrafficView::setDisplayHexAddresses
/// \param on
///
void FormTrafficView::setDisplayHexAddresses(bool on)
{
    _outputWidget->setDisplayHexAddresses(on);

    const auto defs = _mbMultiServer.getModbusDefinitions();
    _lineEditAddress->setInputMode(on ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    _lineEditAddress->setInputRange(ModbusLimits::addressRange(defs.AddrSpace, _comboBoxAddressBase->currentAddressBase() == AddressBase::Base0));
}

///
/// \brief FormTrafficView::captureMode
///
CaptureMode FormTrafficView::captureMode() const
{
    return _outputWidget->captureMode();
}

///
/// \brief FormTrafficView::startTextCapture
/// \param file
///
void FormTrafficView::startTextCapture(const QString& file)
{
    _outputWidget->startTextCapture(file);
}

///
/// \brief FormTrafficView::stopTextCapture
///
void FormTrafficView::stopTextCapture()
{
    _outputWidget->stopTextCapture();
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
            _outputWidget->setDataDisplayMode(DataDisplayMode::Binary);
            break;
        case QModbusDataUnit::InputRegisters:
        case QModbusDataUnit::HoldingRegisters:
            _outputWidget->setDataDisplayMode(mode);
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
    return *_outputWidget->byteOrder();
}

///
/// \brief FormTrafficView::setByteOrder
/// \param order
///
void FormTrafficView::setByteOrder(ByteOrder order)
{
    _outputWidget->setByteOrder(order);
    emit byteOrderChanged(order);
}

///
/// \brief FormTrafficView::codepage
/// \return
///
QString FormTrafficView::codepage() const
{
    return _outputWidget->codepage();
}

///
/// \brief FormTrafficView::setCodepage
/// \param name
///
void FormTrafficView::setCodepage(const QString& name)
{
    _outputWidget->setCodepage(name);
    emit codepageChanged(name);
}

///
/// \brief FormTrafficView::backgroundColor
/// \return
///
QColor FormTrafficView::backgroundColor() const
{
    return _outputWidget->backgroundColor();
}

///
/// \brief FormTrafficView::setBackgroundColor
/// \param clr
///
void FormTrafficView::setBackgroundColor(const QColor& clr)
{
    _outputWidget->setBackgroundColor(clr);
}

///
/// \brief FormTrafficView::foregroundColor
/// \return
///
QColor FormTrafficView::foregroundColor() const
{
    return _outputWidget->foregroundColor();
}

///
/// \brief FormTrafficView::setForegroundColor
/// \param clr
///
void FormTrafficView::setForegroundColor(const QColor& clr)
{
    _outputWidget->setForegroundColor(clr);
}

///
/// \brief FormTrafficView::statusColor
/// \return
///
QColor FormTrafficView::statusColor() const
{
    return _outputWidget->statusColor();
}

///
/// \brief FormTrafficView::setStatusColor
/// \param clr
///
void FormTrafficView::setStatusColor(const QColor& clr)
{
    _outputWidget->setStatusColor(clr);
}

///
/// \brief FormTrafficView::font
/// \return
///
QFont FormTrafficView::font() const
{
   return _outputWidget->font();
}

///
/// \brief FormTrafficView::setFont
/// \param font
///
void FormTrafficView::setFont(const QFont& font)
{
    _outputWidget->setFont(font);
}

///
/// \brief FormTrafficView::zoomPercent
/// \return
///
int FormTrafficView::zoomPercent() const
{
    return _outputWidget->zoomPercent();
}

///
/// \brief FormTrafficView::setZoomPercent
/// \param zoomPercent
///
void FormTrafficView::setZoomPercent(int zoomPercent)
{
    _outputWidget->setZoomPercent(zoomPercent);
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

    const auto textAddrLen = QString(tr("Address Base: %1\nStart Address: %2\nLength: %3")).arg(_comboBoxAddressBase->currentText(), _lineEditAddress->text(), _lineEditLength->text());
    auto rcAddrLen = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextWordWrap, textAddrLen);

    const auto textDevIdType = QString(tr("Unit Identifier: %1\nMODBUS Point Type:\n%2")).arg(_lineEditDeviceId->text(), _comboBoxModbusPointType->currentText());
    auto rcDevIdType = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextWordWrap, textDevIdType);

    rcTime.moveTopRight({ pageRect.right(), 10 });
    rcDevIdType.moveTopLeft({ rcAddrLen.right() + 40, rcAddrLen.top()});

    painter.drawText(rcTime, Qt::TextSingleLine, textTime);
    painter.drawText(rcAddrLen, Qt::TextWordWrap, textAddrLen);
    painter.drawText(rcDevIdType, Qt::TextWordWrap, textDevIdType);

    auto rcOutput = pageRect;
    rcOutput.setTop(rcAddrLen.bottom() + 20);

    _outputWidget->paint(rcOutput, painter);
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
    const auto deviceId = _lineEditDeviceId->value<quint8>();
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
    return _outputWidget->descriptionMap();
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
    _outputWidget->setDescription(deviceId, type, addr, desc);
}

///
/// \brief FormTrafficView::colorMap
/// \return
///
AddressColorMap FormTrafficView::colorMap() const
{
    return _outputWidget->colorMap();
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
    _outputWidget->setColor(deviceId, type, addr, clr);
}

///
/// \brief FormTrafficView::resetCtrs
///
void FormTrafficView::resetCtrs()
{
    _statisticWidget->resetCtrs();
}

///
/// \brief FormTrafficView::requestCount
/// \return
///
uint FormTrafficView::requestCount() const
{
    return _statisticWidget->numberRequets();
}

///
/// \brief FormTrafficView::responseCount
/// \return
///
uint FormTrafficView::responseCount() const
{
    return _statisticWidget->numberResposes();
}

///
/// \brief FormTrafficView::setStatisticCounters
/// \param requests
/// \param responses
///
void FormTrafficView::setStatisticCounters(uint requests, uint responses)
{
    _statisticWidget->setCounters(requests, responses);
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
    return _statisticWidget->logState();
}

///
/// \brief FormTrafficView::setLogViewState
/// \param state
///
void FormTrafficView::setLogViewState(LogViewState state)
{
    _statisticWidget->setLogState(state);
    _outputWidget->setLogViewState(state);
    syncTrafficFilterState(state);
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
    const bool zeroBased = (_comboBoxAddressBase->currentAddressBase() == AddressBase::Base0);
    const int address = _lineEditAddress->value<int>();
    const auto lenRange = ModbusLimits::lengthRange(address, zeroBased, defs.AddrSpace);

    _lineEditLength->setInputRange(lenRange);
    if(_lineEditLength->value<int>() > lenRange.to()) {
        _lineEditLength->setValue(lenRange.to());
        _lineEditLength->update();
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
            _outputWidget->setStatus(QString());
        else
            _outputWidget->setInvalidLengthStatus();
    }
    else
    {
        _outputWidget->setNotConnectedStatus();
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
    _outputWidget->setup(dd, _dataSimulator->simulationMap(), _mbMultiServer.data(dd.DeviceId, dd.PointType, addr, dd.Length));
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
    const auto deviceId = _lineEditDeviceId->value<quint8>();
    const auto pointType = _comboBoxModbusPointType->currentPointType();
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
    _outputWidget->clearLogView();

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
    if((_verboseLogging || isLoggingRequest(msg)) && matchesTrafficFilter(msg)) {
        _statisticWidget->increaseRequests();
        _outputWidget->updateTraffic(msg);
    }
}

///
/// \brief FormTrafficView::on_mbResponse
/// \param msgReq
/// \param msgResp
///
void FormTrafficView::on_mbResponse(QSharedPointer<const ModbusMessage> msgReq, QSharedPointer<const ModbusMessage> msgResp)
{
    const bool logByDefinition = _verboseLogging || isLoggingRequest(msgReq);
    const bool passesFilter = msgReq ? matchesTrafficFilter(msgReq) : matchesTrafficFilter(msgResp);
    if(logByDefinition && passesFilter) {
        _statisticWidget->increaseResponses();
        _outputWidget->updateTraffic(msgResp);
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
        _outputWidget->updateData(_mbMultiServer.data(deviceId, dd.PointType, addr, dd.Length));
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
    _lineEditAddress->setInputRange(ModbusLimits::addressRange(defs.AddrSpace, dd.ZeroBasedAddress));
    _lineEditLength->setInputRange(ModbusLimits::lengthRange(dd.PointAddress, dd.ZeroBasedAddress, defs.AddrSpace));
    _outputWidget->setup(dd, _dataSimulator->simulationMap(), _mbMultiServer.data(dd.DeviceId, dd.PointType, addr, dd.Length));
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
    if(deviceId != _lineEditDeviceId->value<quint8>())
        return;

    for(auto&& addr : addresses)
        _outputWidget->setSimulated(mode, deviceId, type, addr, true);
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
    if(deviceId != _lineEditDeviceId->value<quint8>())
        return;

    for(auto&& addr : addresses)
        _outputWidget->setSimulated(mode, deviceId, type, addr, false);
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
    ui->toolBarScript->clear();

    _scriptRunModeCombo = new RunModeComboBox(ui->toolBarScript);
    _scriptRunModeCombo->setCurrentRunMode(_scriptSettings.Mode);

    _scriptIntervalSpin = new QSpinBox(ui->toolBarScript);
    _scriptIntervalSpin->setRange(500, 10000);
    _scriptIntervalSpin->setSingleStep(100);
    _scriptIntervalSpin->setSuffix(tr(" ms"));
    _scriptIntervalSpin->setValue(static_cast<int>(_scriptSettings.Interval));
    _scriptIntervalSpin->setFixedWidth(90);

    _scriptRunOnStartupCheck = new QCheckBox(tr("Run on startup"), ui->toolBarScript);
    _scriptRunOnStartupCheck->setChecked(_scriptSettings.RunOnStartup);

    ui->toolBarScript->addWidget(_scriptRunModeCombo);
    ui->toolBarScript->addWidget(_scriptIntervalSpin);
    ui->toolBarScript->addWidget(_scriptRunOnStartupCheck);
    ui->toolBarScript->addSeparator();
    ui->toolBarScript->addAction(ui->actionRunScript);
    ui->toolBarScript->addAction(ui->actionStopScript);

    connect(_scriptRunModeCombo, &RunModeComboBox::runModeChanged, this, [this](RunMode mode) {
        _scriptSettings.Mode = mode;
    });

    connect(_scriptIntervalSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int value) {
        _scriptSettings.Interval = static_cast<uint>(value);
    });

    connect(_scriptRunOnStartupCheck, &QCheckBox::stateChanged, this, [this](int state) {
        _scriptSettings.RunOnStartup = (state == Qt::Checked);
    });

    if (auto* runButton = qobject_cast<QToolButton*>(ui->toolBarScript->widgetForAction(ui->actionRunScript)))
        runButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    if (auto* stopButton = qobject_cast<QToolButton*>(ui->toolBarScript->widgetForAction(ui->actionStopScript)))
        stopButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    connect(ui->actionRunScript, &QAction::triggered, this, &FormTrafficView::runScript);
    connect(ui->actionStopScript, &QAction::triggered, this, &FormTrafficView::stopScript);

    connect(this, &FormModSim::scriptRunning, this, &FormTrafficView::updateScriptBar);
    connect(this, &FormModSim::scriptStopped, this, &FormTrafficView::updateScriptBar);
    connect(ui->scriptControl->scriptDocument(), &QTextDocument::contentsChanged,
            this, &FormTrafficView::updateScriptBar);

    updateScriptBar();
}

///
/// \brief FormTrafficView::setupTrafficFilterBar
///
void FormTrafficView::setupTrafficFilterBar()
{
    _toolBarTrafficFilter->clear();

    _labelUnitId = new QLabel(_toolBarTrafficFilter);
    _unitIdFilter = new QSpinBox(_toolBarTrafficFilter);
    _unitIdFilter->setRange(0, 247);
    _unitIdFilter->setValue(0);
    _unitIdFilter->setFixedWidth(70);

    _labelFuncCode = new QLabel(_toolBarTrafficFilter);
    _funcCodeFilter = new QComboBox(_toolBarTrafficFilter);
    _funcCodeFilter->addItem(tr("All"), 0);
    _funcCodeFilter->addItem("FC01 Read Coils", 1);
    _funcCodeFilter->addItem("FC02 Read Discrete Inputs", 2);
    _funcCodeFilter->addItem("FC03 Read Holding Registers", 3);
    _funcCodeFilter->addItem("FC04 Read Input Registers", 4);
    _funcCodeFilter->addItem("FC05 Write Single Coil", 5);
    _funcCodeFilter->addItem("FC06 Write Single Register", 6);
    _funcCodeFilter->addItem("FC15 Write Multiple Coils", 15);
    _funcCodeFilter->addItem("FC16 Write Multiple Registers", 16);
    _funcCodeFilter->setFixedWidth(220);

    _labelRowLimit = new QLabel(_toolBarTrafficFilter);
    _rowLimitCombo = new QComboBox(_toolBarTrafficFilter);
    for (const int limit : {100, 500, 1000, 5000})
        _rowLimitCombo->addItem(QString::number(limit), limit);
    _rowLimitCombo->setCurrentIndex(2);
    _rowLimitCombo->setFixedWidth(65);

    _trafficFilterStretch = new QWidget(_toolBarTrafficFilter);
    _trafficFilterStretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    _pauseButton = new QToolButton(_toolBarTrafficFilter);
    _pauseButton->setCheckable(true);
    _pauseButton->setIcon(QIcon(":/res/actionStopScript.png"));
    _pauseButton->setAutoRaise(true);

    _clearButton = new QToolButton(_toolBarTrafficFilter);
    _clearButton->setIcon(QIcon(":/res/edit-delete.svg"));
    _clearButton->setAutoRaise(true);

    _toolBarTrafficFilter->addWidget(_labelUnitId);
    _toolBarTrafficFilter->addWidget(_unitIdFilter);
    _toolBarTrafficFilter->addSeparator();
    _toolBarTrafficFilter->addWidget(_labelFuncCode);
    _toolBarTrafficFilter->addWidget(_funcCodeFilter);
    _toolBarTrafficFilter->addSeparator();
    _toolBarTrafficFilter->addWidget(_labelRowLimit);
    _toolBarTrafficFilter->addWidget(_rowLimitCombo);
    _toolBarTrafficFilter->addWidget(_trafficFilterStretch);
    _toolBarTrafficFilter->addWidget(_pauseButton);
    _toolBarTrafficFilter->addWidget(_clearButton);

    _outputWidget->setLogViewLimit(_rowLimitCombo->currentData().toInt());

    connect(_rowLimitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        _outputWidget->setLogViewLimit(_rowLimitCombo->currentData().toInt());
    });
    connect(_pauseButton, &QToolButton::toggled, this, [this](bool paused) {
        if (logViewState() == LogViewState::Unknown) {
            _pauseButton->blockSignals(true);
            _pauseButton->setChecked(false);
            _pauseButton->blockSignals(false);
            return;
        }

        const auto state = paused ? LogViewState::Paused : LogViewState::Running;
        setLogViewState(state);
        emit statisticLogStateChanged(state);
    });
    connect(_clearButton, &QToolButton::clicked, this, [this]() {
        _outputWidget->clearLogView();
    });

    retranslateTrafficFilterBar();
}

///
/// \brief FormTrafficView::retranslateTrafficFilterBar
///
void FormTrafficView::retranslateTrafficFilterBar()
{
    if (!_labelUnitId || !_unitIdFilter || !_labelFuncCode || !_labelRowLimit || !_clearButton || !_funcCodeFilter)
        return;

    _labelUnitId->setText(tr("Unit ID:"));
    _unitIdFilter->setSpecialValueText(tr("All"));
    _unitIdFilter->setToolTip(tr("Filter by Unit Identifier (0 = all)"));
    _labelFuncCode->setText(tr("Function:"));
    _labelRowLimit->setText(tr("Limit:"));
    _clearButton->setToolTip(tr("Clear"));
    _funcCodeFilter->setItemText(0, tr("All"));

    syncTrafficFilterState(logViewState());
}

///
/// \brief FormTrafficView::syncTrafficFilterState
/// \param state
///
void FormTrafficView::syncTrafficFilterState(LogViewState state)
{
    if (!_pauseButton)
        return;

    _pauseButton->setEnabled(state != LogViewState::Unknown);
    _pauseButton->blockSignals(true);
    _pauseButton->setChecked(state == LogViewState::Paused);
    _pauseButton->blockSignals(false);
    _pauseButton->setToolTip((state == LogViewState::Paused) ? tr("Resume") : tr("Pause"));
}

///
/// \brief FormTrafficView::updateScriptBar
///
void FormTrafficView::updateScriptBar()
{
    const bool running = canStopScript();
    ui->actionRunScript->setEnabled(canRunScript());
    ui->actionStopScript->setEnabled(running);
    if (_scriptRunModeCombo)
        _scriptRunModeCombo->setEnabled(!running);
    if (_scriptIntervalSpin)
        _scriptIntervalSpin->setEnabled(!running);
    if (_scriptRunOnStartupCheck)
        _scriptRunOnStartupCheck->setEnabled(!running);
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






