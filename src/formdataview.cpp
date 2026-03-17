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
#include "formdataview.h"
#include "ui_formdataview.h"

namespace {
int dataFormId(const QWidget* widget)
{
    if (!widget)
        return -1;
    const QString title = widget->windowTitle();
    int idx = title.size() - 1;
    while (idx >= 0 && title.at(idx).isDigit())
        --idx;
    bool ok = false;
    const int id = title.mid(idx + 1).toInt(&ok);
    return ok ? id : -1;
}
}

///
/// \brief FormDataView::FormDataView
/// \param num
/// \param parent
///
FormDataView::FormDataView(int id, ModbusMultiServer& server, DataSimulator* simulator, MainWindow* parent)
    : QWidget(parent)
    , ui(new Ui::FormDataView)
    ,_parent(parent)
    ,_mbMultiServer(server)
    ,_dataSimulator(simulator)
{
    Q_ASSERT(parent != nullptr);
    Q_ASSERT(_dataSimulator != nullptr);

    ui->setupUi(this);
    setWindowTitle(QString("Data%1").arg(id));
    setWindowIcon(QIcon(":/res/actionShowData.png"));

    ui->lineEditDeviceId->setInputRange(ModbusLimits::slaveRange());
    ui->lineEditDeviceId->setValue(1);
    ui->lineEditDeviceId->setLeadingZeroes(true);
    ui->lineEditDeviceId->setHexButtonVisible(true);
    server.addDeviceId(ui->lineEditDeviceId->value<int>());

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

    connect(this, &FormDataView::definitionChanged, this, &FormDataView::onDefinitionChanged);
    emit definitionChanged();

    ui->outputWidget->setFocus();

    connect(&_mbMultiServer, &ModbusMultiServer::connected, this, &FormDataView::on_mbConnected);
    connect(&_mbMultiServer, &ModbusMultiServer::disconnected, this, &FormDataView::on_mbDisconnected);
    connect(&_mbMultiServer, &ModbusMultiServer::dataChanged, this, &FormDataView::on_mbDataChanged);
    connect(&_mbMultiServer, &ModbusMultiServer::definitionsChanged, this, &FormDataView::on_mbDefinitionsChanged);

    connect(_dataSimulator, &DataSimulator::simulationStarted, this, &FormDataView::on_simulationStarted);
    connect(_dataSimulator, &DataSimulator::simulationStopped, this, &FormDataView::on_simulationStopped);
    connect(_dataSimulator, &DataSimulator::dataSimulated, this, &FormDataView::on_dataSimulated);

    setupDisplayBar();
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
    _mbMultiServer.removeUnitMap(dataFormId(this), deviceId);

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
    return QWidget::mouseDoubleClickEvent(event);
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
DataViewDefinitions FormDataView::displayDefinition() const
{
    DataViewDefinitions dd;
    dd.FormName = windowTitle();
    dd.DeviceId = ui->lineEditDeviceId->value<int>();
    dd.PointAddress = ui->lineEditAddress->value<int>();
    dd.PointType = ui->comboBoxModbusPointType->currentPointType();
    dd.Length = ui->lineEditLength->value<int>();
    dd.ZeroBasedAddress = ui->lineEditAddress->range<int>().from() == 0;
    dd.LogViewLimit = 30;
    dd.AutoscrollLog = false;
    dd.VerboseLogging = true;
    dd.HexAddress = displayHexAddresses();
    dd.HexViewAddress  = ui->lineEditAddress->hexView();
    dd.HexViewDeviceId = ui->lineEditDeviceId->hexView();
    dd.HexViewLength   = ui->lineEditLength->hexView();
    dd.AddrSpace = _mbMultiServer.getModbusDefinitions().AddrSpace;
    dd.DataViewColumnsDistance = ui->outputWidget->dataViewColumnsDistance();
    dd.LeadingZeros = ui->lineEditDeviceId->leadingZeroes();

    return dd;
}

///
/// \brief FormDataView::setDisplayDefinition
/// \param dd
///
void FormDataView::setDisplayDefinition(const DataViewDefinitions& dd)
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
    ui->outputWidget->setDataViewColumnsDistance(dd.DataViewColumnsDistance);

    setDisplayHexAddresses(dd.HexAddress);

    ui->lineEditDeviceId->setHexView(dd.HexViewDeviceId);
    ui->lineEditAddress->setHexView(dd.HexViewAddress);
    ui->lineEditLength->setHexView(dd.HexViewLength);

    emit definitionChanged();
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
    _mbMultiServer.addUnitMap(dataFormId(this), dd.DeviceId, dd.PointType, addr, dd.Length);

    ui->outputWidget->setup(dd, _dataSimulator->simulationMap(), _mbMultiServer.data(dd.DeviceId, dd.PointType, addr, dd.Length));
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
            params.DataMode = mode;
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
            params.DataMode = mode;
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
}

///
/// \brief FormDataView::on_mbDisconnected
///
void FormDataView::on_mbDisconnected(const ConnectionDetails&)
{
    updateStatus();
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
/// \brief FormDataView::setupDisplayBar
///
void FormDataView::setupDisplayBar()
{
    auto group = new QActionGroup(ui->toolBarDisplay);
    group->setExclusive(true);

    auto addModeAction = [&](DataDisplayMode mode, QAction* action)
    {
        group->addAction(action);
        _displayModeActions[mode] = action;
        connect(action, &QAction::triggered, this, [this, mode](bool checked) {
            if (checked) setDataDisplayMode(mode);
        });
    };

    addModeAction(DataDisplayMode::Binary, ui->actionDisplayBinary);
    addModeAction(DataDisplayMode::Hex, ui->actionDisplayHex);
    addModeAction(DataDisplayMode::Ansi, ui->actionDisplayAnsi);
    addModeAction(DataDisplayMode::Int16, ui->actionDisplayInt16);
    addModeAction(DataDisplayMode::UInt16, ui->actionDisplayUInt16);
    addModeAction(DataDisplayMode::Int32, ui->actionDisplayInt32);
    addModeAction(DataDisplayMode::SwappedInt32, ui->actionDisplaySwappedInt32);
    addModeAction(DataDisplayMode::UInt32, ui->actionDisplayUInt32);
    addModeAction(DataDisplayMode::SwappedUInt32, ui->actionDisplaySwappedUInt32);
    addModeAction(DataDisplayMode::Int64, ui->actionDisplayInt64);
    addModeAction(DataDisplayMode::SwappedInt64, ui->actionDisplaySwappedInt64);
    addModeAction(DataDisplayMode::UInt64, ui->actionDisplayUInt64);
    addModeAction(DataDisplayMode::SwappedUInt64, ui->actionDisplaySwappedUInt64);
    addModeAction(DataDisplayMode::FloatingPt, ui->actionDisplayFloatingPt);
    addModeAction(DataDisplayMode::SwappedFP, ui->actionDisplaySwappedFP);
    addModeAction(DataDisplayMode::DblFloat, ui->actionDisplayDblFloat);
    addModeAction(DataDisplayMode::SwappedDbl, ui->actionDisplaySwappedDbl);

    _ansiMenu = new AnsiMenu(this);
    connect(_ansiMenu, &AnsiMenu::codepageSelected, this, &FormDataView::setCodepage);
    ui->actionDisplayAnsi->setMenu(_ansiMenu);
    if (auto* ansiButton = qobject_cast<QToolButton*>(ui->toolBarDisplay->widgetForAction(ui->actionDisplayAnsi))) {
        ansiButton->setPopupMode(QToolButton::DelayedPopup);
    }

    connect(ui->actionDisplaySwapBytes, &QAction::triggered, this, [this](bool checked) {
        setByteOrder(checked ? ByteOrder::Swapped : ByteOrder::Direct);
    });

    updateDisplayBar();
}

///
/// \brief FormDataView::updateDisplayBar
///
void FormDataView::updateDisplayBar()
{
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
    ui->actionDisplaySwapBytes->setChecked(byteOrder() == ByteOrder::Swapped);
    ui->actionDisplaySwapBytes->setEnabled(!coilType);

    // Sync ANSI codepage menu
    if (_ansiMenu) _ansiMenu->selectCodepage(codepage());
}

///
/// \brief FormDataView::connectEditSlots
///
void FormDataView::connectEditSlots()
{
    // Data view has no script editor.
}

///
/// \brief FormDataView::disconnectEditSlots
///
void FormDataView::disconnectEditSlots()
{
    // Data view has no script editor.
}






