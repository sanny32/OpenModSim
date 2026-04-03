#include <QPainter>
#include <QPalette>
#include <QDateTime>
#include <QHelpEngine>
#include <QHelpContentWidget>
#include <QCheckBox>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStyle>
#include "modbuslimits.h"
#include "findreplacebar.h"
#include "mainwindow.h"
#include "modbusmessages.h"
#include "datasimulator.h"
#include "dialogwritestatusregister.h"
#include "dialogwriteregister.h"
#include "formdataview.h"
#include "ui_formdataview.h"

namespace {
constexpr const char* kFormIdProperty = "FormId";
constexpr int kBitPointLengthLimit = 2000;

bool isBitPointType(QModbusDataUnit::RegisterType type)
{
    return type == QModbusDataUnit::Coils || type == QModbusDataUnit::DiscreteInputs;
}

QRange<int> lengthRangeForPointType(int address,
                                    bool zeroBased,
                                    AddressSpace space,
                                    QModbusDataUnit::RegisterType pointType)
{
    const auto defaultRange = ModbusLimits::lengthRange(address, zeroBased, space);
    if(!isBitPointType(pointType))
        return defaultRange;

    const int offset = address - (zeroBased ? 0 : 1);
    const int maxByAddress = ModbusLimits::addressSpaceSize(space) - offset;
    const int maxLen = qMin(kBitPointLengthLimit, maxByAddress);
    return { defaultRange.from(), qMax(defaultRange.from(), maxLen) };
}

QUuid dataFormId(const QWidget* widget)
{
    if (!widget)
        return {};

    return widget->property(kFormIdProperty).toUuid();
}
}

///
/// \brief FormDataView::FormDataView
/// \param parent
///
FormDataView::FormDataView(ModbusMultiServer& server, DataSimulator* simulator, MainWindow* parent)
    : QWidget(parent)
    , ui(new Ui::FormDataView)
    ,_parent(parent)
    ,_mbMultiServer(server)
    ,_dataSimulator(simulator)
{
    Q_ASSERT(parent != nullptr);
    Q_ASSERT(_dataSimulator != nullptr);

    ui->setupUi(this);
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

    ui->lineEditLength->setInputRange(lengthRangeForPointType(0, true, mbDefs.AddrSpace, QModbusDataUnit::HoldingRegisters));
    ui->lineEditLength->setValue(100);
    ui->lineEditLength->setHexButtonVisible(true);

    ui->comboBoxAddressBase->setCurrentAddressBase(AddressBase::Base1);
    ui->comboBoxModbusPointType->setCurrentPointType(QModbusDataUnit::HoldingRegisters);

    connect(this, &FormDataView::definitionChanged, this, &FormDataView::onDefinitionChanged);
    emit definitionChanged();

    _findReplaceBar = new FindReplaceBar(this);
    _findReplaceBar->setReplaceEnabled(false);
    _findReplaceBar->setSearchOptionsVisible(false);
    _findReplaceBar->setWindowedMode(true);
    connect(_findReplaceBar, &FindReplaceBar::searchTextChanged, this, [this](const QString& text) {
        ui->outputWidget->applyFindByValue(text);
    });
    connect(_findReplaceBar, &FindReplaceBar::findNext, this, [this](const QString& text) {
        ui->outputWidget->findNextByValue(text);
    });
    connect(_findReplaceBar, &FindReplaceBar::findPrevious, this, [this](const QString& text) {
        ui->outputWidget->findPreviousByValue(text);
    });
    connect(_findReplaceBar, &FindReplaceBar::closed, this, [this]() {
        ui->outputWidget->clearFindByValue();
        ui->outputWidget->setFocus();
    });

    connect(ui->outputWidget, &OutputDataWidget::colorChanged, this, &FormDataView::colorChanged);
    connect(ui->outputWidget, &OutputDataWidget::descriptionChanged, this,
            [this](quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc) {
        _mbMultiServer.setDescription(deviceId, type, addr, desc);
        emit descriptionChanged(deviceId, type, addr, desc);
    });

    ui->outputWidget->installEventFilter(this);
    ui->outputWidget->setFocus();

    connect(&_mbMultiServer, &ModbusMultiServer::connected, this, [this](const ConnectionDetails&) { updateStatus(); });
    connect(&_mbMultiServer, &ModbusMultiServer::disconnected, this, [this](const ConnectionDetails&) { updateStatus(); });
    connect(&_mbMultiServer, &ModbusMultiServer::dataChanged, this, &FormDataView::on_mbDataChanged);
    connect(&_mbMultiServer, &ModbusMultiServer::descriptionChanged, this, &FormDataView::on_mbDescriptionChanged);
    connect(&_mbMultiServer, &ModbusMultiServer::definitionsChanged, this, &FormDataView::on_mbDefinitionsChanged);

    connect(_dataSimulator, &DataSimulator::simulationStarted, this, &FormDataView::on_simulationStarted);
    connect(_dataSimulator, &DataSimulator::simulationStopped, this, &FormDataView::on_simulationStopped);
    connect(_dataSimulator, &DataSimulator::dataSimulated, this, &FormDataView::on_dataSimulated);

    setupDisplayBar();

    const auto sa = ui->frameDataDefinition;
    sa->horizontalScrollBar()->installEventFilter(this);
    const int contentH = sa->widget()->minimumSizeHint().height();
    sa->setMinimumHeight(contentH + 2 * sa->frameWidth());
}

///
/// \brief FormDataView::~FormDataView
///
FormDataView::~FormDataView()
{
    const auto deviceId = ui->lineEditDeviceId->value<quint8>();
    _mbMultiServer.removeDeviceId(deviceId);
    _mbMultiServer.removeUnitMap(dataFormId(this), deviceId);
    delete ui;
}

///
/// \brief FormDataView::saveSettings
/// \param out
///
void FormDataView::saveSettings(QSettings& out) const
{
    out << const_cast<FormDataView*>(this);
}

///
/// \brief FormDataView::loadSettings
/// \param in
///
void FormDataView::loadSettings(QSettings& in)
{
    in >> this;
}

///
/// \brief FormDataView::saveXml
/// \param xml
///
void FormDataView::saveXml(QXmlStreamWriter& xml) const
{
    xml << const_cast<FormDataView*>(this);
}

///
/// \brief FormDataView::loadXml
/// \param xml
///
void FormDataView::loadXml(QXmlStreamReader& xml)
{
    xml >> this;
}

///
/// \brief FormDataView::eventFilter
/// \param obj
/// \param event
/// \return
///
bool FormDataView::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui->frameDataDefinition->horizontalScrollBar())
    {
        const auto type = event->type();
        if (type == QEvent::Show || type == QEvent::Hide)
        {
            const auto sa = ui->frameDataDefinition;
            const int contentH = sa->widget()->minimumSizeHint().height();
            const int frameH = 2 * sa->frameWidth();
            const int scrollH = (type == QEvent::Show) ? sa->horizontalScrollBar()->sizeHint().height() : 0;
            sa->setMinimumHeight(contentH + frameH + scrollH);
        }
    }
    else if (obj == ui->outputWidget && event->type() == QEvent::Resize)
    {
        if (_findReplaceBar)
            _findReplaceBar->updatePosition();
    }
    return QWidget::eventFilter(obj, event);
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
    emit closing();
    QWidget::closeEvent(event);
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

    {
        QSignalBlocker b(ui->comboBoxAddressBase);
        ui->comboBoxAddressBase->setCurrentAddressBase(dd.ZeroBasedAddress ? AddressBase::Base0 : AddressBase::Base1);
    }
    {
        QSignalBlocker b(ui->lineEditAddress);
        ui->lineEditAddress->setLeadingZeroes(dd.LeadingZeros);
        ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(defs.AddrSpace, dd.ZeroBasedAddress));
        ui->lineEditAddress->setValue(dd.PointAddress);
    }
    {
        QSignalBlocker b(ui->lineEditLength);
        ui->lineEditLength->setLeadingZeroes(dd.LeadingZeros);
        ui->lineEditLength->setInputRange(lengthRangeForPointType(dd.PointAddress, dd.ZeroBasedAddress, defs.AddrSpace, dd.PointType));
        ui->lineEditLength->setValue(dd.Length);
    }
    {
        QSignalBlocker b(ui->comboBoxModbusPointType);
        ui->comboBoxModbusPointType->setCurrentPointType(dd.PointType);
    }
    ui->outputWidget->setDataViewColumnsDistance(dd.DataViewColumnsDistance);

    setDisplayHexAddresses(dd.HexAddress);

    ui->lineEditDeviceId->setHexView(dd.HexViewDeviceId);
    ui->lineEditAddress->setHexView(dd.HexViewAddress);
    ui->lineEditLength->setHexView(dd.HexViewLength);

    updateSettingsControls();
    emit definitionChanged();
}

///
/// \brief FormDataView::dataType
/// \return
///
DataType FormDataView::dataType() const
{
    return ui->outputWidget->dataType();
}

///
/// \brief FormDataView::registerOrder
/// \return
///
RegisterOrder FormDataView::registerOrder() const
{
    return ui->outputWidget->registerOrder();
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
    if(displayHexAddresses() == on) return;
    ui->outputWidget->setDisplayHexAddresses(on);

    const auto defs = _mbMultiServer.getModbusDefinitions();
    ui->lineEditAddress->setInputMode(on ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(defs.AddrSpace, ui->comboBoxAddressBase->currentAddressBase() == AddressBase::Base0));
    updateSettingsControls();
    emit displayHexAddressesChanged(on);
}

///
/// \brief FormDataView::setDataType
/// \param type
///
void FormDataView::setDataType(DataType type)
{
    const auto prevType = dataType();
    const auto dd = displayDefinition();
    switch(dd.PointType) {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
            ui->outputWidget->setDataType(DataType::Binary);
            break;
        case QModbusDataUnit::InputRegisters:
        case QModbusDataUnit::HoldingRegisters:
            ui->outputWidget->setDataType(type);
            break;
        default: break;
    }
    updateDisplayBar();
    reapplyFind();
    if(dataType() != prevType)
        emit dataTypeChanged(dataType());
}

///
/// \brief FormDataView::setRegisterOrder
/// \param order
///
void FormDataView::setRegisterOrder(RegisterOrder order)
{
    const auto prev = registerOrder();
    ui->outputWidget->setRegisterOrder(order);
    updateDisplayBar();
    reapplyFind();
    if(registerOrder() != prev)
        emit registerOrderChanged(registerOrder());
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
    if(byteOrder() == order) return;
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
    if(codepage() == name) return;
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
    if(backgroundColor() == clr) return;
    ui->outputWidget->setBackgroundColor(clr);
    emit backgroundColorChanged(clr);
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
    if(foregroundColor() == clr) return;
    ui->outputWidget->setForegroundColor(clr);
    emit foregroundColorChanged(clr);
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
    if(statusColor() == clr) return;
    ui->outputWidget->setStatusColor(clr);
    emit statusColorChanged(clr);
}

///
/// \brief FormDataView::addressColor
/// \return
///
QColor FormDataView::addressColor() const
{
    return ui->outputWidget->addressColor();
}

///
/// \brief FormDataView::setAddressColor
/// \param clr
///
void FormDataView::setAddressColor(const QColor& clr)
{
    if(addressColor() == clr) return;
    ui->outputWidget->setAddressColor(clr);
    emit addressColorChanged(clr);
}

///
/// \brief FormDataView::commentColor
/// \return
///
QColor FormDataView::commentColor() const
{
    return ui->outputWidget->commentColor();
}

///
/// \brief FormDataView::setCommentColor
/// \param clr
///
void FormDataView::setCommentColor(const QColor& clr)
{
    if(commentColor() == clr) return;
    ui->outputWidget->setCommentColor(clr);
    emit commentColorChanged(clr);
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
    if(this->font() == font) return;
    ui->outputWidget->setFont(font);
    emit fontChanged(font);
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

    // Header content is the same for all pages — compute once
    const auto textTime = QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat);
    const auto textAddrLen = QString(tr("Address Base: %1\nStarting Address: %2\nLength: %3"))
        .arg(ui->comboBoxAddressBase->currentText(), ui->lineEditAddress->text(), ui->lineEditLength->text());
    const auto textDevIdType = QString(tr("Unit Identifier: %1\nData Type:\n%2"))
        .arg(ui->lineEditDeviceId->text(), ui->comboBoxModbusPointType->currentText());

    // Header geometry is also the same for all pages — compute once
    auto rcTime    = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextSingleLine, textTime);
    auto rcAddrLen = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextWordWrap,   textAddrLen);
    auto rcDevIdType = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextWordWrap, textDevIdType);
    rcTime.moveTopRight({ pageRect.right(), cy });
    rcDevIdType.moveTopLeft({ rcAddrLen.right() + 40, rcAddrLen.top() });

    const int lineGap = qMax(pageRect.height() / 60, 10);  // ~1.6% page height, min 10px
    auto rcOutput = pageRect;
    rcOutput.setTop(rcAddrLen.bottom() + lineGap * 2);

    // Separator line position — centered between header block and data area
    const int lineY = rcAddrLen.bottom() + lineGap;

    const int totalRows = ui->outputWidget->rowCount();
    int startRow = 0;
    do
    {
        painter.drawText(rcTime,      Qt::TextSingleLine, textTime);
        painter.drawText(rcAddrLen,   Qt::TextWordWrap,   textAddrLen);
        painter.drawText(rcDevIdType, Qt::TextWordWrap,   textDevIdType);
        painter.drawLine(pageRect.left(), lineY, pageRect.right(), lineY);

        startRow = ui->outputWidget->paint(rcOutput, painter, startRow);

        if (startRow < totalRows)
            printer->newPage();

    } while (startRow < totalRows);
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
    for(auto it = simulationMap.cbegin(); it != simulationMap.cend(); ++it)
    {
        if(it.value().Mode == SimulationMode::Disabled)
            continue;

        const auto& key = it.key();
        if(key.DeviceId == dd.DeviceId &&
           key.Type == dd.PointType &&
           key.Address >= startAddr && key.Address < endAddr)
        {
            result[key] = it.value();
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
///
/// \brief FormDataView::serializeModbusDataUnit
///
QModbusDataUnit FormDataView::serializeModbusDataUnit(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 startAddress, quint16 length) const
{
    QModbusDataUnit dataUnit;
    const auto serverData = _mbMultiServer.data(deviceId, type, startAddress, length);

    if (startAddress >= serverData.startAddress() &&
        (startAddress + length) <= (serverData.startAddress() + serverData.valueCount())) {

        const int offset = startAddress - serverData.startAddress();

        dataUnit.setValues(serverData.values().mid(offset, length));
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
    AddressDescriptionMap2 result;
    const auto dd = displayDefinition();
    const auto startAddress = static_cast<quint16>(dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1));
    const auto map = _mbMultiServer.descriptionMap(dd.DeviceId, dd.PointType, startAddress, dd.Length);
    for(auto it = map.constBegin(); it != map.constEnd(); ++it)
        result.insert({dd.DeviceId, it.key().first, it.key().second}, it.value());
    return result;
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
    _mbMultiServer.setDescription(deviceId, type, addr, desc);

    const auto dd = displayDefinition();
    if(deviceId == dd.DeviceId && type == dd.PointType) {
        const auto startAddress = static_cast<quint16>(dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1));
        const quint32 endAddress = static_cast<quint32>(startAddress) + dd.Length;
        if(addr >= startAddress && static_cast<quint32>(addr) < endAddress) {
            QSignalBlocker blocker(ui->outputWidget);
            ui->outputWidget->setDescription(deviceId, type, addr, desc);
        }
    }
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
/// \brief FormDataView::setDisplayDefinitionSilent
/// Updates all definition controls and server registration without emitting definitionChanged.
/// Used for peer sync to prevent signal loops.
///
void FormDataView::setDisplayDefinitionSilent(const DataViewDefinitions& dd)
{
    if(!dd.FormName.isEmpty())
        setWindowTitle(dd.FormName);

    const auto defs = _mbMultiServer.getModbusDefinitions();

    // Handle device ID change (reference counting in server).
    {
        QSignalBlocker b(ui->lineEditDeviceId);
        const auto oldId = ui->lineEditDeviceId->value<quint8>();
        const auto newId = static_cast<quint8>(dd.DeviceId);
        if(oldId != newId) {
            _mbMultiServer.removeDeviceId(oldId);
            _mbMultiServer.addDeviceId(newId);
        }
        ui->lineEditDeviceId->setLeadingZeroes(dd.LeadingZeros);
        ui->lineEditDeviceId->setValue(dd.DeviceId);
        ui->lineEditDeviceId->setHexView(dd.HexViewDeviceId);
    }

    {
        QSignalBlocker b(ui->comboBoxAddressBase);
        ui->comboBoxAddressBase->setCurrentAddressBase(dd.ZeroBasedAddress ? AddressBase::Base0 : AddressBase::Base1);
    }
    {
        QSignalBlocker b(ui->lineEditAddress);
        ui->lineEditAddress->setLeadingZeroes(dd.LeadingZeros);
        ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(defs.AddrSpace, dd.ZeroBasedAddress));
        ui->lineEditAddress->setValue(dd.PointAddress);
        ui->lineEditAddress->setHexView(dd.HexViewAddress);
    }
    {
        QSignalBlocker b(ui->lineEditLength);
        ui->lineEditLength->setLeadingZeroes(dd.LeadingZeros);
        ui->lineEditLength->setInputRange(lengthRangeForPointType(dd.PointAddress, dd.ZeroBasedAddress, defs.AddrSpace, dd.PointType));
        ui->lineEditLength->setValue(dd.Length);
        ui->lineEditLength->setHexView(dd.HexViewLength);
    }
    {
        QSignalBlocker b(ui->comboBoxModbusPointType);
        ui->comboBoxModbusPointType->setCurrentPointType(dd.PointType);
    }

    ui->outputWidget->setDataViewColumnsDistance(dd.DataViewColumnsDistance);
    setDisplayHexAddresses(dd.HexAddress);
    updateSettingsControls();

    // Re-register unit map and refresh output widget.
    onDefinitionChanged();
    updateDisplayBar();
}

///
/// \brief FormDataView::setLeadingZerosEnabled
///
void FormDataView::setLeadingZerosEnabled(bool on)
{
    if(ui->lineEditDeviceId->leadingZeroes() == on &&
       ui->lineEditAddress->leadingZeroes() == on &&
       ui->lineEditLength->leadingZeroes() == on)
    {
        return;
    }

    const auto deviceId = ui->lineEditDeviceId->value<int>();
    const auto address = ui->lineEditAddress->value<int>();
    const auto length = ui->lineEditLength->value<int>();

    ui->lineEditDeviceId->setLeadingZeroes(on);
    ui->lineEditAddress->setLeadingZeroes(on);
    ui->lineEditLength->setLeadingZeroes(on);

    // Force text refresh with the same numeric values.
    ui->lineEditDeviceId->setValue(deviceId);
    ui->lineEditAddress->setValue(address);
    ui->lineEditLength->setValue(length);

    updateSettingsControls();
}

///
/// \brief FormDataView::setColumnsDistance
///
void FormDataView::setColumnsDistance(int value)
{
    const int normalized = qBound(1, value, 32);
    if(ui->outputWidget->dataViewColumnsDistance() == normalized)
        return;

    ui->outputWidget->setDataViewColumnsDistance(normalized);
    updateSettingsControls();
}

///
/// \brief FormDataView::linkTo
/// Bidirectionally syncs display definition, display mode, byte order, and codepage with \a other.
///
void FormDataView::linkTo(FormDataView* other)
{
    if(!other) return;
    connect(this, &FormDataView::definitionChanged, other, [this, other]() {
        other->setDisplayDefinitionSilent(displayDefinition());
    });
    connect(other, &FormDataView::definitionChanged, this, [this, other]() {
        setDisplayDefinitionSilent(other->displayDefinition());
    });
    connect(this,  &FormDataView::byteOrderChanged, other, &FormDataView::setByteOrder);
    connect(other, &FormDataView::byteOrderChanged, this,  &FormDataView::setByteOrder);
    connect(this,  &FormDataView::codepageChanged, other, &FormDataView::setCodepage);
    connect(other, &FormDataView::codepageChanged, this,  &FormDataView::setCodepage);
    connect(this,  &FormDataView::dataTypeChanged,      other, &FormDataView::setDataType);
    connect(other, &FormDataView::dataTypeChanged,      this,  &FormDataView::setDataType);
    connect(this,  &FormDataView::registerOrderChanged, other, &FormDataView::setRegisterOrder);
    connect(other, &FormDataView::registerOrderChanged, this,  &FormDataView::setRegisterOrder);
    connect(this,  &FormDataView::displayHexAddressesChanged, other, &FormDataView::setDisplayHexAddresses);
    connect(other, &FormDataView::displayHexAddressesChanged, this,  &FormDataView::setDisplayHexAddresses);
    connect(this,  &FormDataView::fontChanged, other, &FormDataView::setFont);
    connect(other, &FormDataView::fontChanged, this,  &FormDataView::setFont);
    connect(this,  &FormDataView::foregroundColorChanged, other, &FormDataView::setForegroundColor);
    connect(other, &FormDataView::foregroundColorChanged, this,  &FormDataView::setForegroundColor);
    connect(this,  &FormDataView::backgroundColorChanged, other, &FormDataView::setBackgroundColor);
    connect(other, &FormDataView::backgroundColorChanged, this,  &FormDataView::setBackgroundColor);
    connect(this,  &FormDataView::addressColorChanged, other, &FormDataView::setAddressColor);
    connect(other, &FormDataView::addressColorChanged, this,  &FormDataView::setAddressColor);
    connect(this,  &FormDataView::commentColorChanged, other, &FormDataView::setCommentColor);
    connect(other, &FormDataView::commentColorChanged, this,  &FormDataView::setCommentColor);
    connect(this,  &FormDataView::statusColorChanged, other, &FormDataView::setStatusColor);
    connect(other, &FormDataView::statusColorChanged, this,  &FormDataView::setStatusColor);
    connect(this, &FormDataView::colorChanged, other,
        [other](quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QColor& clr) {
            QSignalBlocker blocker(other);
            other->setColor(deviceId, type, addr, clr);
        });
    connect(other, &FormDataView::colorChanged, this,
        [this](quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QColor& clr) {
            QSignalBlocker blocker(this);
            this->setColor(deviceId, type, addr, clr);
        });
    connect(this, &FormDataView::descriptionChanged, other,
        [other](quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc) {
            QSignalBlocker blocker(other);
            other->setDescription(deviceId, type, addr, desc);
        });
    connect(other, &FormDataView::descriptionChanged, this,
        [this](quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc) {
            QSignalBlocker blocker(this);
            this->setDescription(deviceId, type, addr, desc);
        });
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
/// \brief FormDataView::showFind
///
void FormDataView::showFind()
{
    if (!_findReplaceBar)
        return;

    _findReplaceBar->showFind();

    const QPoint outputTopLeft = ui->outputWidget->mapTo(this, QPoint(0, 0));
    const int frameWidth = ui->outputWidget->frameWidth();
    const int x = outputTopLeft.x() + ui->outputWidget->width() - _findReplaceBar->width() - frameWidth;
    const int y = outputTopLeft.y() + frameWidth;
    _findReplaceBar->move(qMax(0, x), qMax(0, y));
}

///
/// \brief FormDataView::on_lineEditAddress_valueChanged
///
void FormDataView::on_lineEditAddress_valueChanged(const QVariant&)
{
    const auto defs = _mbMultiServer.getModbusDefinitions();
    const bool zeroBased = (ui->comboBoxAddressBase->currentAddressBase() == AddressBase::Base0);
    const int address = ui->lineEditAddress->value<int>();
    const auto lenRange = lengthRangeForPointType(address, zeroBased, defs.AddrSpace, ui->comboBoxModbusPointType->currentPointType());

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
    const auto defs = _mbMultiServer.getModbusDefinitions();
    const bool zeroBased = (ui->comboBoxAddressBase->currentAddressBase() == AddressBase::Base0);
    const int address = ui->lineEditAddress->value<int>();
    const auto lenRange = lengthRangeForPointType(address, zeroBased, defs.AddrSpace, type);

    ui->lineEditLength->setInputRange(lenRange);
    if(ui->lineEditLength->value<int>() > lenRange.to()) {
        ui->lineEditLength->setValue(lenRange.to());
        ui->lineEditLength->update();
    }

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
    syncDescriptionsFromServer();
    updateDisplayBar();
    reapplyFind();
}

void FormDataView::reapplyFind()
{
    if (_findReplaceBar && !_findReplaceBar->searchText().trimmed().isEmpty())
        ui->outputWidget->applyFindByValue(_findReplaceBar->searchText());
}

void FormDataView::syncDescriptionsFromServer()
{
    const auto dd = displayDefinition();
    const auto startAddress = static_cast<quint16>(dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1));
    const auto map = _mbMultiServer.descriptionMap(dd.DeviceId, dd.PointType, startAddress, dd.Length);

    QSignalBlocker blocker(ui->outputWidget);
    for(auto it = map.constBegin(); it != map.constEnd(); ++it)
        ui->outputWidget->setDescription(dd.DeviceId, dd.PointType, it.key().second, it.value());
}

///
/// \brief FormDataView::on_outputWidget_itemDoubleClicked
/// \param addr
/// \param value
///
void FormDataView::on_outputWidget_itemDoubleClicked(quint16 addr, const QVariant& value)
{
    const auto dd = displayDefinition();
    const auto pointType = ui->comboBoxModbusPointType->currentPointType();

    ModbusWriteParams params;
    params.DeviceId = ui->lineEditDeviceId->value<quint8>();
    params.Address = addr;
    params.Value = value;
    params.DataMode = dataType();
    params.RegOrder = registerOrder();
    params.AddrSpace = _mbMultiServer.getModbusDefinitions().AddrSpace;
    params.Order = byteOrder();
    params.Codepage = codepage();
    params.ZeroBasedAddress = dd.ZeroBasedAddress;
    params.LeadingZeros = dd.LeadingZeros;
    params.Server = &_mbMultiServer;

    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
        {
            DialogWriteStatusRegister dlg(params, pointType, displayHexAddresses(), _dataSimulator, _parent);
            if(dlg.exec() == QDialog::Accepted)
                _mbMultiServer.writeRegister(pointType, params);
        }
        break;

        case QModbusDataUnit::InputRegisters:
        case QModbusDataUnit::HoldingRegisters:
        {
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
/// \brief FormDataView::on_mbDataChanged
///
void FormDataView::on_mbDataChanged(quint8 deviceId, const QModbusDataUnit&)
{
    const auto dd = displayDefinition();
    if(deviceId == dd.DeviceId)
    {
        const auto addr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
        ui->outputWidget->updateData(_mbMultiServer.data(deviceId, dd.PointType, addr, dd.Length));
        reapplyFind();
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
    ui->lineEditLength->setInputRange(lengthRangeForPointType(dd.PointAddress, dd.ZeroBasedAddress, defs.AddrSpace, dd.PointType));
    ui->outputWidget->setup(dd, _dataSimulator->simulationMap(), _mbMultiServer.data(dd.DeviceId, dd.PointType, addr, dd.Length));
    syncDescriptionsFromServer();
    reapplyFind();
}

///
/// \brief FormDataView::on_mbDescriptionChanged
/// \param deviceId
/// \param type
/// \param addr
/// \param desc
///
void FormDataView::on_mbDescriptionChanged(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc)
{
    const auto dd = displayDefinition();
    if(deviceId != dd.DeviceId || type != dd.PointType)
        return;

    const auto startAddress = static_cast<quint16>(dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1));
    const quint32 endAddress = static_cast<quint32>(startAddress) + dd.Length;
    if(addr < startAddress || static_cast<quint32>(addr) >= endAddress)
        return;

    QSignalBlocker blocker(ui->outputWidget);
    ui->outputWidget->setDescription(deviceId, type, addr, desc);
}

///
/// \brief FormDataView::on_simulationStarted
/// \param mode
/// \param deviceId
/// \param type
/// \param addresses
///
void FormDataView::on_simulationStarted(DataType type, RegisterOrder order, quint8 deviceId, QModbusDataUnit::RegisterType regType, const QVector<quint16>& addresses)
{
    if(deviceId != ui->lineEditDeviceId->value<quint8>())
        return;

    for(auto&& addr : addresses)
        ui->outputWidget->setSimulated(type, order, deviceId, regType, addr, true);
}

///
/// \brief FormDataView::on_simulationStopped
/// \param type
/// \param order
/// \param deviceId
/// \param regType
/// \param addresses
///
void FormDataView::on_simulationStopped(DataType type, RegisterOrder order, quint8 deviceId, QModbusDataUnit::RegisterType regType, const QVector<quint16>& addresses)
{
    if(deviceId != ui->lineEditDeviceId->value<quint8>())
        return;

    for(auto&& addr : addresses)
        ui->outputWidget->setSimulated(type, order, deviceId, regType, addr, false);
}

///
/// \brief FormDataView::on_dataSimulated
///
void FormDataView::on_dataSimulated(DataType type, RegisterOrder order, quint8 deviceId, QModbusDataUnit::RegisterType regType, quint16 startAddress, QVariant value)
{
    const auto dd = displayDefinition();
    const auto pointAddr = dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1);
    if(deviceId == dd.DeviceId && regType == dd.PointType && startAddress >= pointAddr && startAddress <= pointAddr + dd.Length)
    {
        const ModbusWriteParams params = { dd.DeviceId, startAddress, value, type, order, dd.AddrSpace, byteOrder(), codepage(), true };
        _mbMultiServer.writeRegister(regType, params);
    }
}

///
/// \brief FormDataView::setupDisplayBar
///
void FormDataView::setupDisplayBar()
{
    auto group = new QActionGroup(ui->toolBarDisplay);
    group->setExclusive(true);

    auto modeHint = [](DataType type, RegisterOrder order)
    {
        const auto typeName = enumToString(type);
        if (isMultiRegisterType(type))
            return QStringLiteral("%1 (%2)").arg(typeName, enumToString(order));
        return typeName;
    };

    auto addModeAction = [&](DataType type, RegisterOrder order, QAction* action)
    {
        group->addAction(action);
        _displayModeActions[{type, order}] = action;

        const auto hint = modeHint(type, order);
        action->setToolTip(hint);
        action->setStatusTip(hint);
        action->setWhatsThis(hint);

        connect(action, &QAction::triggered, this, [this, type, order](bool checked) {
            if (checked) { setDataType(type); setRegisterOrder(order); }
        });
    };

    const auto msrf = RegisterOrder::MSRF;
    const auto lsrf = RegisterOrder::LSRF;

    addModeAction(DataType::Binary,  msrf, ui->actionDisplayBinary);
    addModeAction(DataType::Hex,     msrf, ui->actionDisplayHex);
    addModeAction(DataType::Ansi,    msrf, ui->actionDisplayAnsi);
    addModeAction(DataType::Int16,   msrf, ui->actionDisplayInt16);
    addModeAction(DataType::UInt16,  msrf, ui->actionDisplayUInt16);
    addModeAction(DataType::Int32,   msrf, ui->actionDisplayInt32);
    addModeAction(DataType::Int32,   lsrf, ui->actionDisplayInt32LSRF);
    addModeAction(DataType::UInt32,  msrf, ui->actionDisplayUInt32);
    addModeAction(DataType::UInt32,  lsrf, ui->actionDisplayUInt32LSRF);
    addModeAction(DataType::Int64,   msrf, ui->actionDisplayInt64);
    addModeAction(DataType::Int64,   lsrf, ui->actionDisplayInt64LSRF);
    addModeAction(DataType::UInt64,  msrf, ui->actionDisplayUInt64);
    addModeAction(DataType::UInt64,  lsrf, ui->actionDisplayUInt64LSRF);
    addModeAction(DataType::Float32, msrf, ui->actionDisplayFloat32);
    addModeAction(DataType::Float32, lsrf, ui->actionDisplayFloat32LSRF);
    addModeAction(DataType::Float64, msrf, ui->actionDisplayFloat64);
    addModeAction(DataType::Float64, lsrf, ui->actionDisplayFloat64LSRF);

    _ansiMenu = new AnsiMenu(this);
    connect(_ansiMenu, &AnsiMenu::codepageSelected, this, &FormDataView::setCodepage);
    ui->actionDisplayAnsi->setMenu(_ansiMenu);
    if (auto* ansiButton = qobject_cast<QToolButton*>(ui->toolBarDisplay->widgetForAction(ui->actionDisplayAnsi))) {
        ansiButton->setPopupMode(QToolButton::DelayedPopup);
    }

    connect(ui->actionDisplaySwapBytes, &QAction::triggered, this, [this](bool checked) {
        setByteOrder(checked ? ByteOrder::Swapped : ByteOrder::Direct);
    });

    connect(ui->actionDisplayHexAddresses, &QAction::triggered, this, [this](bool checked) {
        setDisplayHexAddresses(checked);
        emit definitionChanged();
    });

    // Settings controls
    connect(ui->checkBoxHexAddress, &QCheckBox::toggled, this, [this](bool checked) {
        setDisplayHexAddresses(checked);
        emit definitionChanged();
    });
    connect(ui->checkBoxLeadingZeros, &QCheckBox::toggled, this, [this](bool checked) {
        setLeadingZerosEnabled(checked);
        emit definitionChanged();
    });
    connect(ui->spinBoxColumnsDistance, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        setColumnsDistance(value);
        emit definitionChanged();
    });

    updateDisplayBar();
}

///
/// \brief FormDataView::updateSettingsControls
///
void FormDataView::updateSettingsControls()
{
    {
        QSignalBlocker blocker(ui->checkBoxHexAddress);
        ui->checkBoxHexAddress->setChecked(displayHexAddresses());
    }

    {
        QSignalBlocker blocker(ui->actionDisplayHexAddresses);
        ui->actionDisplayHexAddresses->setChecked(displayHexAddresses());
    }

    {
        QSignalBlocker blocker(ui->checkBoxLeadingZeros);
        ui->checkBoxLeadingZeros->setChecked(ui->lineEditDeviceId->leadingZeroes());
    }

    {
        QSignalBlocker blocker(ui->spinBoxColumnsDistance);
        ui->spinBoxColumnsDistance->setValue(ui->outputWidget->dataViewColumnsDistance());
    }
}

///
/// \brief FormDataView::updateDisplayBar
///
void FormDataView::updateDisplayBar()
{
    const auto dd = displayDefinition();
    const bool coilType = (dd.PointType == QModbusDataUnit::Coils ||
                           dd.PointType == QModbusDataUnit::DiscreteInputs);

    // Check the matching mode action
    const auto it = _displayModeActions.find({dataType(), registerOrder()});
    if (it != _displayModeActions.end())
        it.value()->setChecked(true);

    // Enable/disable non-binary actions for coil/discrete types
    const auto* binaryAction = _displayModeActions.value({DataType::Binary, RegisterOrder::MSRF});
    for (auto&& action : _displayModeActions) {
        action->setEnabled(!coilType || action == binaryAction);
    }

    // Sync swap bytes
    ui->actionDisplaySwapBytes->setChecked(byteOrder() == ByteOrder::Swapped);
    ui->actionDisplaySwapBytes->setEnabled(!coilType);

    // Sync ANSI codepage menu
    if (_ansiMenu) _ansiMenu->selectCodepage(codepage());

    updateSettingsControls();
}

///
/// \brief FormDataView::connectEditSlots
///
void FormDataView::connectEditSlots()
{
    disconnectEditSlots();
    connect(_parent, &MainWindow::find, this, &FormDataView::showFind);
}

///
/// \brief FormDataView::disconnectEditSlots
///
void FormDataView::disconnectEditSlots()
{
    disconnect(_parent, &MainWindow::find, this, &FormDataView::showFind);
}






