#include <QDateTime>
#include <QPainter>
#include <QInputDialog>
#include "fontutils.h"
#include "formatutils.h"
#include "datadelegate.h"
#include "outputdatawidget.h"
#include "ui_outputdatawidget.h"

///
/// \brief SimulationRole
///
const int SimulationRole = Qt::UserRole + 1;

///
/// \brief CaptureRole
///
const int CaptureRole = Qt::UserRole + 2;

///
/// \brief DescriptionRole
///
const int DescriptionRole = Qt::UserRole + 3;

///
/// \brief AddressStringRole
///
const int AddressStringRole = Qt::UserRole + 4;

///
/// \brief AddressRole
///
const int AddressRole = Qt::UserRole + 5;

///
/// \brief ValueRole
///
const int ValueRole = Qt::UserRole + 6;

///
/// \brief ColorRole
///
const int ColorRole = Qt::UserRole + 7;

///
/// \brief emptyPixmap
/// \param size
/// \return
///
static QPixmap emptyPixmap(const QSize& size)
{
    QPixmap pm(size);
    pm.fill(Qt::transparent);
    return pm;
}

///
/// \brief OutputDataListModel::OutputDataListModel
/// \param parent
///
OutputDataListModel::OutputDataListModel(OutputDataWidget* parent)
    : QAbstractListModel(parent)
    ,_parentWidget(parent)
    ,_iconSimulation16Bit(QIcon(":/res/icon-simulation-16bit.svg").pixmap(10, 10))
    ,_iconSimulation32Bit(QIcon(":/res/icon-simulation-32bit.svg").pixmap(10, 10))
    ,_iconSimulation64Bit(QIcon(":/res/icon-simulation-64bit.svg").pixmap(10, 10))
    ,_iconSimulationOff(emptyPixmap(_iconSimulation16Bit.size()))
{
}

///
/// \brief OutputDataListModel::rowCount
/// \return
///
int OutputDataListModel::rowCount(const QModelIndex&) const
{
    return _parentWidget->_displayDefinition.Length;
}

///
/// \brief OutputDataListModel::data
/// \param index
/// \param role
/// \return
///
QVariant OutputDataListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    const auto row = index.row();
    const auto pointType = _parentWidget->_displayDefinition.PointType;
    const auto addrSpace = _parentWidget->_displayDefinition.AddrSpace;
    const auto hexAddresses = _parentWidget->displayHexAddresses();

    const auto itemData = _mapItems.find(row);
    if (itemData == _mapItems.end())
    {
        return QVariant();
    }

    switch(role)
    {
        case Qt::DisplayRole:
        {
            QString descr;
            if (!itemData->Description.isEmpty())
            {
                const auto freeSpace = _columnsDistance - 2;
                if (freeSpace > 0)
                {
                    descr = QStringLiteral("; ");
                    if (itemData->Description.length() > freeSpace)
                    {
                        if (freeSpace > 3)
                        {
                            descr += itemData->Description.left(freeSpace - 3);
                            descr += QStringLiteral("...");
                        }
                    }
                    else
                    {
                        descr += itemData->Description;
                    }
                }
            }
            const auto pad = _columnsDistance - descr.length();
            return formatAddress(pointType, itemData->Address, addrSpace, hexAddresses) +
                QStringLiteral(": ") + itemData->ValueStr + descr + QStringLiteral(" ").repeated(pad);
        }

        case CaptureRole:
            return QString(itemData->ValueStr).remove('<').remove('>');

        case AddressStringRole:
            return formatAddress(pointType, itemData->Address, addrSpace, hexAddresses);

        case AddressRole:
            return itemData->Address;

        case ValueRole:
            return itemData->Value;

        case ColorRole:
            return itemData->BgColor;

        case DescriptionRole:
            return itemData->Description;

        case Qt::DecorationRole:
        {
            if(itemData->ValueStr.isEmpty())
                return _iconSimulationOff;

            switch(simulationIcon(row))
            {
                case SimulationIcon16Bit:
                    return _iconSimulation16Bit;
                case SimulationIcon32Bit:
                    return _iconSimulation32Bit;
                case SimulationIcon64Bit:
                    return _iconSimulation64Bit;

                default:
                    return _iconSimulationOff;
            }
        }
    }

    return QVariant();
}

///
/// \brief OutputDataListModel::setData
/// \param index
/// \param value
/// \param role
/// \return
///
bool OutputDataListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
    {
        return false;
    }
    const auto itemData = _mapItems.find(index.row());
    if (itemData == _mapItems.end())
    {
        return false;
    }

    switch (role)
    {
        case SimulationRole:
            itemData->Simulated = value.toBool();
            emit dataChanged(index, index, QVector<int>() << role);
        return true;

        case Qt::DecorationRole:
            itemData->SimulationIcon = value.value<SimulationIconType>();
            emit dataChanged(index, index, QVector<int>() << role);
        return true;

        case DescriptionRole:
            itemData->Description = value.toString();
            emit dataChanged(index, index, QVector<int>() << role);
        return true;

        case ColorRole:
            itemData->BgColor = value.value<QColor>();
            emit dataChanged(index, index, QVector<int>() << Qt::DisplayRole);
        return true;

        default:
        return false;
    }
}

///
/// \brief OutputDataListModel::isUpdated
/// \return
///
bool OutputDataListModel::isValid() const
{
    return _lastData.isValid();
}

///
/// \brief OutputDataListModel::values
/// \return
///
QVector<quint16> OutputDataListModel::values() const
{
    return _lastData.values();
}

///
/// \brief OutputDataListModel::clear
///
void OutputDataListModel::clear()
{
    _mapItems.clear();
    updateData(QModbusDataUnit());
}

///
/// \brief OutputDataListModel::update
///
void OutputDataListModel::update()
{
    updateData(_lastData);
}

///
/// \brief OutputDataListModel::updateData
/// \param data
///
void OutputDataListModel::updateData(const QModbusDataUnit& data)
{
    const auto mode = _parentWidget->dataDisplayMode();
    const auto leadingZeros = _parentWidget->_displayDefinition.LeadingZeros;
    const auto pointType = _parentWidget->_displayDefinition.PointType;
    const auto byteOrder = *_parentWidget->byteOrder();
    const auto codepage = _parentWidget->codepage();

    const bool all_changed =
        !_lastData.isValid()
        || (_lastMode != mode)
        || (_lastLeadingZeros != leadingZeros)
        || (_lastPointType != pointType)
        || (_lastByteOrder != byteOrder)
        || (_lastCodepage != codepage);

    int first_changed = -1, last_changed = -1;

    for(int i = 0; i < rowCount(); i++)
    {
        const auto value = data.value(i);

        auto& itemData = _mapItems[i];
        itemData.Address = _parentWidget->_displayDefinition.PointAddress + i;

        // Detect the changed data
        if (!all_changed && !itemData.ValueStr.isEmpty())
        {
            bool skip = true;
            for (int reg = 0; reg < registersCount(mode); ++reg)
            {
                if (data.value(i + reg) != _lastData.value(i + reg))
                {
                    skip = false;
                    break;
                }
            }
            if (skip)
            {
                continue;
            }
        }

        // Adjust the range of changed data
        if (first_changed == -1)
        {
            first_changed = i;
        }
        last_changed = i;

        switch(mode)
        {
            case DataDisplayMode::Binary:
                itemData.ValueStr = formatBinaryValue(pointType, value, byteOrder, itemData.Value);
            break;

            case DataDisplayMode::UInt16:
                itemData.ValueStr = formatUInt16Value(pointType, value, byteOrder, leadingZeros, itemData.Value);
            break;

            case DataDisplayMode::Int16:
                itemData.ValueStr = formatInt16Value(pointType, value, byteOrder, itemData.Value);
            break;

            case DataDisplayMode::Hex:
                itemData.ValueStr = formatHexValue(pointType, value, byteOrder, itemData.Value);
            break;

            case DataDisplayMode::Ansi:
                itemData.ValueStr = formatAnsiValue(pointType, value, byteOrder, codepage, itemData.Value);
                break;

            case DataDisplayMode::FloatingPt:
                // MSRF
                itemData.ValueStr = formatFloatValue(pointType, data.value(i+1), value, byteOrder,
                                                     (i%2) || (i+1>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::SwappedFP:
                // LSRF
                itemData.ValueStr = formatFloatValue(pointType, value, data.value(i+1), byteOrder,
                                                     (i%2) || (i+1>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::DblFloat:
                // MSRF
                itemData.ValueStr = formatDoubleValue(pointType, data.value(i+3), data.value(i+2), data.value(i+1), value,
                                                      byteOrder, (i%4) || (i+3>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::SwappedDbl:
                // LSRF
                itemData.ValueStr = formatDoubleValue(pointType, value, data.value(i+1), data.value(i+2), data.value(i+3),
                                                      byteOrder, (i%4) || (i+3>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::Int32:
                // MSRF
                itemData.ValueStr = formatInt32Value(pointType, data.value(i+1), value, byteOrder,
                                                     (i%2) || (i+1>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::SwappedInt32:
                // LSRF
                itemData.ValueStr = formatInt32Value(pointType, value, data.value(i+1), byteOrder,
                                                     (i%2) || (i+1>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::UInt32:
                // MSRF
                itemData.ValueStr = formatUInt32Value(pointType, data.value(i+1), value, byteOrder, leadingZeros,
                                                      (i%2) || (i+1>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::SwappedUInt32:
                // LSRF
                itemData.ValueStr = formatUInt32Value(pointType, value, data.value(i+1), byteOrder, leadingZeros,
                                                      (i%2) || (i+1>=rowCount()), itemData.Value);
                break;

            case DataDisplayMode::Int64:
                // MSRF
                itemData.ValueStr = formatInt64Value(pointType, data.value(i+3), data.value(i+2), data.value(i+1), value,
                                                     byteOrder, (i%4) || (i+3>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::SwappedInt64:
                // LSRF
                itemData.ValueStr = formatInt64Value(pointType, value, data.value(i+1), data.value(i+2), data.value(i+3),
                                                     byteOrder, (i%4) || (i+3>=rowCount()), itemData.Value);
                break;

            case DataDisplayMode::UInt64:
                // MSRF
                itemData.ValueStr = formatUInt64Value(pointType, data.value(i+3), data.value(i+2), data.value(i+1), value,
                                                      byteOrder, leadingZeros, (i%4) || (i+3>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::SwappedUInt64:
                // LSRF
                itemData.ValueStr = formatUInt64Value(pointType, value, data.value(i+1), data.value(i+2), data.value(i+3),
                                                      byteOrder, leadingZeros, (i%4) || (i+3>=rowCount()), itemData.Value);
            break;
        }
    }

    if (all_changed || first_changed != -1)
    {
        _lastData = data;
        _lastMode = mode;
        _lastLeadingZeros = leadingZeros;
        _lastPointType = pointType;
        _lastByteOrder = byteOrder;
        _lastCodepage = codepage;

        // Notify changed data only
        if (first_changed != -1)
        {
            emit dataChanged(index(first_changed), index(last_changed), QVector<int>() << Qt::DisplayRole);
        }
    }
}

///
/// \brief OutputDataListModel::find
/// \param deviceId
/// \param type
/// \param addr
/// \return
///
QModelIndex OutputDataListModel::find(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr) const
{
    if(_parentWidget->_displayDefinition.PointType != type || _parentWidget->_displayDefinition.DeviceId != deviceId)
        return QModelIndex();

    const auto dd =  _parentWidget->_displayDefinition;
    const int row = addr - (dd.PointAddress - (dd.ZeroBasedAddress ? 0 : 1));
    if(row >= 0 && row < rowCount())
        return index(row);

    return QModelIndex();
}

///
/// \brief OutputDataListModel::simulationIcon
/// \param row
/// \return
///
OutputDataListModel::SimulationIconType OutputDataListModel::simulationIcon(int row) const
{
    const auto mode = _parentWidget->dataDisplayMode();
    for(int i = 0; i < registersCount(mode); ++i)
    {
        if(row + i >= rowCount())
            return SimulationIconNone;

        const auto itemData = _mapItems.find(row + i);
        if (itemData == _mapItems.end())
            return SimulationIconNone;

        if(itemData->Simulated)
            return itemData->SimulationIcon;
    }

    return SimulationIconNone;
}

///
/// \brief OutputDataWidget::OutputDataWidget
/// \param parent
///
OutputDataWidget::OutputDataWidget(QWidget *parent) :
     QFrame(parent)
   , ui(new Ui::OutputDataWidget)
   ,_displayHexAddreses(false)
   ,_dataDisplayMode(DataDisplayMode::Hex)
   ,_byteOrder(ByteOrder::Direct)
   ,_listModel(new OutputDataListModel(this))
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
    ui->listView->setUniformItemSizes(true);
    ui->listView->setModel(_listModel.get());
    ui->listView->setItemDelegate(new DataDelegate( this ));
    ui->labelStatus->setAutoFillBackground(true);

    setFont(defaultMonospaceFont());
    setAutoFillBackground(true);
    setForegroundColor(Qt::black);
    setBackgroundColor(Qt::white);

    setStatusColor(Qt::red);
    setNotConnectedStatus();

    hideModbusMessage();

    connect(ui->logView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this, [&](const QItemSelection& sel) {
                if(!sel.indexes().isEmpty())
                    showModbusMessage(sel.indexes().first());
            });

    _baseFontSize = ui->listView->font().pointSizeF();
    if (_baseFontSize <= 0) _baseFontSize = 10.0;

    _zoomLabel = new QLabel(this);
    _zoomLabel->setVisible(false);
    _zoomLabel->setAlignment(Qt::AlignCenter);
    _zoomLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    _zoomLabel->setStyleSheet(R"(
        QLabel {
            background: rgba(30, 30, 30, 180);
            color: white;
            border-radius: 6px;
            padding: 6px 12px;
            font-size: 12pt;
        }
    )");

    _zoomHideTimer = new QTimer(this);
    _zoomHideTimer->setSingleShot(true);
    connect(_zoomHideTimer, &QTimer::timeout, _zoomLabel, &QLabel::hide);

    ui->listView->viewport()->installEventFilter(this);
}

///
/// \brief OutputDataWidget::~OutputDataWidget
///
OutputDataWidget::~OutputDataWidget()
{
    delete ui;
}

///
/// \brief OutputDataWidget::changeEvent
/// \param event
///
void OutputDataWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        if(!_listModel->isValid())
            setNotConnectedStatus();
    }

    QWidget::changeEvent(event);
}

///
/// \brief OutputDataWidget::eventFilter
/// \param obj
/// \param event
/// \return
///
bool OutputDataWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui->listView->viewport() &&
        event->type() == QEvent::Wheel)
    {
        auto we = static_cast<QWheelEvent*>(event);
        if (we->modifiers() & Qt::ControlModifier)
        {
            if (we->angleDelta().y() > 0)
                setZoomPercent(zoomPercent() + 10);
            else if (we->angleDelta().y() < 0)
                setZoomPercent(zoomPercent() - 10);

            showZoomOverlay();

            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

///
/// \brief OutputDataWidget::data
/// \return
///
QVector<quint16> OutputDataWidget::data() const
{
    return _listModel->values();
}

///
/// \brief OutputDataWidget::setup
/// \param dd
/// \param simulations
/// \param data
///
void OutputDataWidget::setup(const DataViewDefinitions& dd, const ModbusSimulationMap2& simulations, const QModbusDataUnit& data)
{
    _descriptionMap.insert(descriptionMap());
    _colorMap.insert(colorMap());

    _displayDefinition = dd;
    setDataViewColumnsDistance(dd.DataViewColumnsDistance);
    ui->logView->setShowLeadingZeros(dd.LeadingZeros);
    ui->modbusMsg->setShowLeadingZeros(dd.LeadingZeros);

    _listModel->clear();

    for(auto it = simulations.constBegin(); it != simulations.constEnd(); ++it) {
        const auto index = _listModel->find(it.key().DeviceId, it.key().Type, it.key().Address);
        const auto& params = it.value();
        _listModel->setData(index, true, SimulationRole);
        if(params.Mode != SimulationMode::Disabled) {
            switch(registersCount(params.DataMode)) {
                case 1: _listModel->setData(index, OutputDataListModel::SimulationIcon16Bit, Qt::DecorationRole); break;
                case 2: _listModel->setData(index, OutputDataListModel::SimulationIcon32Bit, Qt::DecorationRole); break;
                case 4: _listModel->setData(index, OutputDataListModel::SimulationIcon64Bit, Qt::DecorationRole); break;
            }
        }
    }

    for(auto&& key : _descriptionMap.keys())
        setDescription(key.DeviceId, key.Type, key.Address, _descriptionMap[key]);

    for(auto&& key : _colorMap.keys())
        setColor(key.DeviceId, key.Type, key.Address, _colorMap[key]);

    updateData(data);
}

///
/// \brief OutputDataWidget::displayHexAddresses
/// \return
///
bool OutputDataWidget::displayHexAddresses() const
{
    return _displayHexAddreses;
}

///
/// \brief OutputDataWidget::setDisplayHexAddresses
/// \param on
///
void OutputDataWidget::setDisplayHexAddresses(bool on)
{
    if (_displayHexAddreses == on)
        return;

    _displayHexAddreses = on;
    if (_listModel->rowCount() > 0) {
        emit _listModel->dataChanged(_listModel->index(0),
                                     _listModel->index(_listModel->rowCount() - 1),
                                     QVector<int>() << Qt::DisplayRole);
    }
}




///
/// \brief OutputDataWidget::backgroundColor
/// \return
///
QColor OutputDataWidget::backgroundColor() const
{
    return ui->listView->palette().color(QPalette::Base);
}

///
/// \brief OutputDataWidget::setBackgroundColor
/// \param clr
///
void OutputDataWidget::setBackgroundColor(const QColor& clr)
{
    auto pal = palette();
    pal.setColor(QPalette::Base, clr);
    pal.setColor(QPalette::Window, clr);
    setPalette(pal);
}

///
/// \brief OutputDataWidget::foregroundColor
/// \return
///
QColor OutputDataWidget::foregroundColor() const
{
    return ui->listView->palette().color(QPalette::Text);
}

///
/// \brief OutputDataWidget::setForegroundColor
/// \param clr
///
void OutputDataWidget::setForegroundColor(const QColor& clr)
{
    auto pal = ui->listView->palette();
    pal.setColor(QPalette::Text, clr);
    ui->listView->setPalette(pal);
}

///
/// \brief OutputDataWidget::statusColor
/// \return
///
QColor OutputDataWidget::statusColor() const
{
    return ui->labelStatus->palette().color(QPalette::WindowText);
}

///
/// \brief OutputDataWidget::setStatusColor
/// \param clr
///
void OutputDataWidget::setStatusColor(const QColor& clr)
{
    auto pal = ui->labelStatus->palette();
    pal.setColor(QPalette::WindowText, clr);
    ui->labelStatus->setPalette(pal);
    ui->modbusMsg->setStatusColor(clr);
}

///
/// \brief OutputDataWidget::font
/// \return
///
QFont OutputDataWidget::font() const
{
    return ui->logView->font();
}

///
/// \brief OutputDataWidget::setFont
/// \param font
///
void OutputDataWidget::setFont(const QFont& font)
{
    _baseFontSize = font.pointSizeF();

    ui->listView->setFont(font);
    ui->labelStatus->setFont(font);
    ui->logView->setFont(font);
    ui->modbusMsg->setFont(font);

    setZoomPercent(_zoomPercent);
}

///
/// \brief OutputDataWidget::zoomPercent
/// \return
///
int OutputDataWidget::zoomPercent() const
{
    return _zoomPercent;
}

///
/// \brief OutputDataWidget::setZoomPercent
/// \param zoom
///
void OutputDataWidget::setZoomPercent(int zoomPercent)
{
    _zoomPercent = qBound(50, zoomPercent, 300);

    QFont font = ui->listView->font();
    font.setPointSizeF(_baseFontSize * _zoomPercent / 100.0);

    ui->listView->setFont(font);
    ui->labelStatus->setFont(font);
}

///
/// \brief OutputDataWidget::dataViewColumnsDistance
/// \return
///
int OutputDataWidget::dataViewColumnsDistance() const
{
    return _listModel->columnsDistance();
}

///
/// \brief OutputDataWidget::setDataViewColumnsDistance
/// \param value
///
void OutputDataWidget::setDataViewColumnsDistance(int value)
{
    _listModel->setColumnsDistance(value);
}







///
/// \brief OutputDataWidget::setStatus
/// \param status
///
void OutputDataWidget::setStatus(const QString& status)
{
    if(status.isEmpty())
    {
        ui->labelStatus->setText(status);
    }
    else
    {
        const auto info = QString("*** %1 ***").arg(status);
        if(info != ui->labelStatus->text())
        {
            ui->labelStatus->setText(info);
        }
    }
}

///
/// \brief OutputDataWidget::paint
/// \param rc
/// \param painter
///
void OutputDataWidget::paint(const QRect& rc, QPainter& painter)
{
    const auto textStatus = ui->labelStatus->text();
    auto rcStatus = painter.boundingRect(rc.left(), rc.top(), rc.width(), rc.height(), Qt::TextWordWrap, textStatus);
    painter.drawText(rcStatus, Qt::TextWordWrap, textStatus);

    rcStatus.setBottom(rcStatus.bottom() + 4);
    painter.drawLine(rc.left(), rcStatus.bottom(), rc.right(), rcStatus.bottom());
    rcStatus.setBottom(rcStatus.bottom() + 4);

    int cx = rc.left();
    int cy = rcStatus.bottom();
    int maxWidth = 0;
    for(int i = 0; i < _listModel->rowCount(); ++i)
    {
        QTextDocument doc;
        doc.setHtml(_listModel->data(_listModel->index(i), Qt::DisplayRole).toString());
        const auto text = doc.toPlainText().trimmed();

        auto rcItem = painter.boundingRect(cx, cy, rc.width() - cx, rc.height() - cy, Qt::TextSingleLine, text);
        maxWidth = qMax(maxWidth, rcItem.width());

        if(rcItem.right() > rc.right()) break;
        else if(rcItem.bottom() < rc.bottom())
        {
            painter.drawText(rcItem, Qt::TextSingleLine, text);
        }
        else
        {
            cy = rcStatus.bottom();
            cx = rcItem.left() + maxWidth + 10;

            rcItem = painter.boundingRect(cx, cy, rc.width() - cx, rc.height() - cy, Qt::TextSingleLine, text);
            if(rcItem.right() > rc.right()) break;

            painter.drawText(rcItem, Qt::TextSingleLine, text);
        }


        cy += rcItem.height();
    }
}


///
/// \brief OutputDataWidget::updateData
///
void OutputDataWidget::updateData(const QModbusDataUnit& data)
{
    _listModel->updateData(data);
}

///
/// \brief OutputDataWidget::colorMap
/// \return
///
AddressColorMap OutputDataWidget::colorMap() const
{
    AddressColorMap colorMap;
    for(int i = 0; i < _listModel->rowCount(); i++)
    {
        const auto clr = _listModel->data(_listModel->index(i), ColorRole).value<QColor>();
        const quint16 addr = _listModel->data(_listModel->index(i), AddressRole).toUInt() - (_displayDefinition.ZeroBasedAddress ? 0 : 1);
        colorMap[{_displayDefinition.DeviceId, _displayDefinition.PointType, addr }] = clr;
    }
    return colorMap;
}

///
/// \brief OutputDataWidget::setColor
/// \param deviceId
/// \param type
/// \param addr
/// \param clr
///
void OutputDataWidget::setColor(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QColor& clr)
{
    _listModel->setData(_listModel->find(deviceId, type, addr), clr, ColorRole);
}

///
/// \brief OutputDataWidget::descriptionMap
/// \return
///
AddressDescriptionMap2 OutputDataWidget::descriptionMap() const
{
    AddressDescriptionMap2 descriptionMap;
    for(int i = 0; i < _listModel->rowCount(); i++)
    {
        const auto desc = _listModel->data(_listModel->index(i), DescriptionRole).toString();
        const quint16 addr = _listModel->data(_listModel->index(i), AddressRole).toUInt() - (_displayDefinition.ZeroBasedAddress ? 0 : 1);
        descriptionMap[{_displayDefinition.DeviceId, _displayDefinition.PointType, addr }] = desc;
    }
    return descriptionMap;
}

///
/// \brief OutputDataWidget::setDescription
/// \param deviceId
/// \param type
/// \param addr
/// \param desc
///
void OutputDataWidget::setDescription(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc)
{
    _listModel->setData(_listModel->find(deviceId, type, addr), desc, DescriptionRole);
}

///
/// \brief OutputDataWidget::setSimulated
/// \param deviceId
/// \param type
/// \param addr
/// \param on
///
void OutputDataWidget::setSimulated(DataDisplayMode mode, quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, bool on)
{
    const auto index = _listModel->find(deviceId, type, addr);
    _listModel->setData(index, on, SimulationRole);

    if(on) {
        switch(registersCount(mode))
        {
            case 1:
                _listModel->setData(index, OutputDataListModel::SimulationIcon16Bit, Qt::DecorationRole);
            break;
            case 2:
                _listModel->setData(index, OutputDataListModel::SimulationIcon32Bit, Qt::DecorationRole);
            break;
            case 4:
                _listModel->setData(index, OutputDataListModel::SimulationIcon64Bit, Qt::DecorationRole);
            break;
        }
    }
    else {
        _listModel->setData(index, OutputDataListModel::SimulationIconNone, Qt::DecorationRole);
    }
}

///
/// \brief OutputDataWidget::dataDisplayMode
/// \return
///
DataDisplayMode OutputDataWidget::dataDisplayMode() const
{
    return _dataDisplayMode;
}

///
/// \brief OutputDataWidget::setDataDisplayMode
/// \param mode
///
void OutputDataWidget::setDataDisplayMode(DataDisplayMode mode)
{
    _dataDisplayMode = mode;
    ui->logView->setDataDisplayMode(mode);
    ui->modbusMsg->setDataDisplayMode(mode);

    _listModel->update();
}

///
/// \brief OutputDataWidget::byteOrder
/// \return
///
const ByteOrder* OutputDataWidget::byteOrder() const
{
    return &_byteOrder;
}

///
/// \brief OutputDataWidget::setByteOrder
/// \param order
///
void OutputDataWidget::setByteOrder(ByteOrder order)
{
    _byteOrder = order;
    ui->modbusMsg->setByteOrder(order);

    _listModel->update();
}

///
/// \brief OutputDataWidget::codepage
/// \return
///
QString OutputDataWidget::codepage() const
{
    return _codepage;
}

///
/// \brief OutputDataWidget::setCodepage
/// \param name
///
void OutputDataWidget::setCodepage(const QString& name)
{
    _codepage = name;
    _listModel->update();
}

///
/// \brief drawRemoveColorIcon
/// \param size
/// \return
///
static QIcon drawRemoveColorIcon(int size = 16)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);

    QRect r(2, 2, size - 4, size - 4);
    p.setBrush(Qt::white);
    p.setPen(QPen(Qt::black, 0.1));
    p.drawRect(r);

    QPen pen(Qt::red, 1);
    p.setPen(pen);
    p.drawLine(0, size, size, 0);

    return QIcon(pm);
}

///
/// \brief OutputDataWidget::on_listView_customContextMenuRequested
/// \param pos
///
void OutputDataWidget::on_listView_customContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->listView->indexAt(pos);
    if (!index.isValid())
        return;

    QMenu menu(this);

    const auto idx = getValueIndex(index);
    const auto address = _listModel->data(idx, AddressStringRole).toString();

    QAction* writeValueAction = menu.addAction(tr("Set Value of %1").arg(address));
    connect(writeValueAction, &QAction::triggered, this, [this, index](){
        emit ui->listView->doubleClicked(index);
    });

    const auto desc = _listModel->data(index, DescriptionRole).toString();
    QAction* addDescrAction = menu.addAction(desc.isEmpty() ? tr("Add Description") : tr("Edit Description"));
    connect(addDescrAction, &QAction::triggered, this, [this, index, address](){
        QInputDialog dlg(this);
        dlg.setLabelText(QString(tr("%1: Enter Description")).arg(_listModel->data(index, AddressStringRole).toString()));
        dlg.setTextValue(_listModel->data(index, DescriptionRole).toString());
        if(dlg.exec() == QDialog::Accepted) {
            _listModel->setData(index, dlg.textValue(), DescriptionRole);
        }
    });

    menu.addSeparator();

    QAction* removeColorAction = menu.addAction(drawRemoveColorIcon(), tr("Remove Color"));
    const auto clr = _listModel->data(index, ColorRole).value<QColor>();
    removeColorAction->setEnabled(clr.isValid());
    connect(removeColorAction, &QAction::triggered, this, [this, index](){
        _listModel->setData(index, QColor(), ColorRole);
    });

    menu.addSeparator();

    struct ColorItem { QString name; QColor color; };
    QVector<ColorItem> safeColors = {
        { tr("Yellow"), QColor(Qt::yellow) },
        { tr("Cyan"), QColor(Qt::cyan) },
        { tr("Magenta"), QColor(Qt::magenta) },
        { tr("LightGreen"), QColor(144, 238, 144) },
        { tr("Orange"), QColor(255, 165, 0) },
        { tr("LightBlue"), QColor(173, 216, 230) },
        { tr("LightGray"), QColor(Qt::lightGray) }
    };

    for (const auto &c : safeColors)
    {
        QPixmap pixmap(16,16);
        pixmap.fill(c.color);
        QIcon icon(pixmap);

        QAction* colorAction = menu.addAction(icon, c.name);
        connect(colorAction, &QAction::triggered, this, [this, index, c](){
            _listModel->setData(index, c.color, ColorRole);
        });
    }

    menu.exec(ui->listView->viewport()->mapToGlobal(pos));
}

///
/// \brief OutputDataWidget::on_listView_doubleClicked
/// \param item
///
void OutputDataWidget::on_listView_doubleClicked(const QModelIndex& index)
{
    if(!index.isValid()) return;

    const auto idx = getValueIndex(index);
    const auto address = _listModel->data(idx, AddressRole).toUInt();
    const auto value = _listModel->data(idx, ValueRole);

    emit itemDoubleClicked(address, value);
}

///
/// \brief OutputDataWidget::getValueIndex
/// \param index
/// \return
///
QModelIndex OutputDataWidget::getValueIndex(const QModelIndex& index) const
{
    QModelIndex idx = index;
    switch(_displayDefinition.PointType)
    {
        case QModbusDataUnit::HoldingRegisters:
        case QModbusDataUnit::InputRegisters:
        {
            switch(_dataDisplayMode)
            {
                case DataDisplayMode::FloatingPt:
                case DataDisplayMode::SwappedFP:
                case DataDisplayMode::Int32:
                case DataDisplayMode::SwappedInt32:
                case DataDisplayMode::UInt32:
                case DataDisplayMode::SwappedUInt32:
                    if(index.row() % 2)
                        idx = _listModel->index(index.row() - 1);
                    break;

                case DataDisplayMode::DblFloat:
                case DataDisplayMode::SwappedDbl:
                case DataDisplayMode::Int64:
                case DataDisplayMode::SwappedInt64:
                case DataDisplayMode::UInt64:
                case DataDisplayMode::SwappedUInt64:
                    if(index.row() % 4)
                        idx = _listModel->index(index.row() - index.row() % 4);
                    break;

                default:
                    break;
            }
        }
        break;

        default:
            break;
    }
    return idx;
}

///
/// \brief OutputDataWidget::setNotConnectedStatus
///
void OutputDataWidget::setNotConnectedStatus()
{
    setStatus(tr("NOT CONNECTED!"));
}

///
/// \brief OutputDataWidget::setInvalidLengthStatus
///
void OutputDataWidget::setInvalidLengthStatus()
{
    setStatus(tr("Invalid Data Length Specified"));
}


///
/// \brief OutputDataWidget::showModbusMessage
/// \param index
///
void OutputDataWidget::showModbusMessage(const QModelIndex& index)
{
    const auto msg = ui->logView->itemAt(index);
    if(msg) {
        if(ui->splitter->widget(1)->isHidden()) {
            ui->splitter->setSizes({1, 1});
            ui->splitter->widget(1)->show();
        }
        ui->modbusMsg->setModbusMessage(msg);
    }
}

///
/// \brief OutputDataWidget::hideModbusMessage
///
void OutputDataWidget::hideModbusMessage()
{
    ui->splitter->setSizes({1, 0});
    ui->splitter->widget(1)->hide();
}

///
/// \brief OutputDataWidget::showZoomOverlay
/// \param currentFontSize
///
void OutputDataWidget::showZoomOverlay()
{
    _zoomLabel->setText(tr("Zoom: %1%").arg(_zoomPercent));
    _zoomLabel->adjustSize();

    if(!_zoomLabel->isVisible())
    {
        const QPoint centerInThis = ui->listView->viewport()->mapTo(this, ui->listView->viewport()->rect().center());
        const QPoint center = centerInThis - QPoint(_zoomLabel->width() / 2, _zoomLabel->height() / 2);

        _zoomLabel->move(center);
        _zoomLabel->show();
        _zoomLabel->raise();
    }

    _zoomHideTimer->start(800);
}




