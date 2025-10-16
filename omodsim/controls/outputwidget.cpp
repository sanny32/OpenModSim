#include <QDateTime>
#include <QPainter>
#include <QTextStream>
#include <QInputDialog>
#include "fontutils.h"
#include "formatutils.h"
#include "outputwidget.h"
#include "ui_outputwidget.h"

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
/// \brief OutputListModel::OutputListModel
/// \param parent
///
OutputListModel::OutputListModel(OutputWidget* parent)
    : QAbstractListModel(parent)
    ,_parentWidget(parent)
    ,_iconPointGreen(QIcon(":/res/pointGreen.png"))
    ,_iconPointEmpty(QIcon(":/res/pointEmpty.png"))
{
}

///
/// \brief OutputListModel::rowCount
/// \return
///
int OutputListModel::rowCount(const QModelIndex&) const
{
    return _parentWidget->_displayDefinition.Length;
}

///
/// \brief OutputListModel::data
/// \param index
/// \param role
/// \return
///
QVariant OutputListModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() ||
       !_mapItems.contains(index.row()))
    {
        return QVariant();
    }

    const auto row = index.row();
    const auto pointType = _parentWidget->_displayDefinition.PointType;
    const auto hexAddresses = _parentWidget->displayHexAddresses();

    const ItemData& itemData = _mapItems[row];
    const auto addrstr = formatAddress(pointType, itemData.Address, hexAddresses);

    switch(role)
    {
        case Qt::DisplayRole:
        {
            auto str = QString("%1: %2").arg(addrstr, itemData.ValueStr);
            const int length = str.length();
            const auto descr = itemData.Description.length() > 20 ?
                        QString("%1...").arg(itemData.Description.left(18)): itemData.Description;
            if(!descr.isEmpty()) str += QString("; %1").arg(descr);
            return str.leftJustified(length + 16, ' ');
        }

        case CaptureRole:
            return QString(itemData.ValueStr).remove('<').remove('>');

        case AddressStringRole:
            return addrstr;

        case AddressRole:
            return itemData.Address;

        case ValueRole:
            return itemData.Value;

        case DescriptionRole:
            return itemData.Description;

        case Qt::DecorationRole:
            return itemData.Simulated ? _iconPointGreen : _iconPointEmpty;
    }

    return QVariant();
}

///
/// \brief OutputListModel::setData
/// \param index
/// \param value
/// \param role
/// \return
///
bool OutputListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid() ||
       !_mapItems.contains(index.row()))
    {
        return false;
    }

    switch (role)
    {
        case SimulationRole:
            _mapItems[index.row()].Simulated = value.toBool();
            emit dataChanged(index, index, QVector<int>() << role);
        return true;


        case DescriptionRole:
            _mapItems[index.row()].Description = value.toString();
            emit dataChanged(index, index, QVector<int>() << role);
        return true;

        default:
        return false;
    }
}

///
/// \brief OutputListModel::isUpdated
/// \return
///
bool OutputListModel::isValid() const
{
    return _lastData.isValid();
}

///
/// \brief OutputListModel::values
/// \return
///
QVector<quint16> OutputListModel::values() const
{
    return _lastData.values();
}

///
/// \brief OutputListModel::clear
///
void OutputListModel::clear()
{
    _mapItems.clear();
    updateData(QModbusDataUnit());
}

///
/// \brief OutputListModel::update
///
void OutputListModel::update()
{
    updateData(_lastData);
}

///
/// \brief OutputListModel::updateData
/// \param data
///
void OutputListModel::updateData(const QModbusDataUnit& data)
{
    _lastData = data;

    const auto mode = _parentWidget->dataDisplayMode();
    const auto pointType = _parentWidget->_displayDefinition.PointType;
    const auto byteOrder = *_parentWidget->byteOrder();
    const auto codepage = _parentWidget->codepage();

    for(int i = 0; i < rowCount(); i++)
    {
        const auto value = _lastData.value(i);

        auto& itemData = _mapItems[i];
        itemData.Address = _parentWidget->_displayDefinition.PointAddress + i;

        switch(mode)
        {
            case DataDisplayMode::Binary:
                itemData.ValueStr = formatBinaryValue(pointType, value, byteOrder, itemData.Value);
            break;

            case DataDisplayMode::UInt16:
                itemData.ValueStr = formatUInt16Value(pointType, value, byteOrder, itemData.Value);
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
                itemData.ValueStr = formatFloatValue(pointType, value, _lastData.value(i+1), byteOrder,
                                          (i%2) || (i+1>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::SwappedFP:
                itemData.ValueStr = formatFloatValue(pointType, _lastData.value(i+1), value, byteOrder,
                                          (i%2) || (i+1>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::DblFloat:
                itemData.ValueStr = formatDoubleValue(pointType, value, _lastData.value(i+1), _lastData.value(i+2), _lastData.value(i+3),
                                           byteOrder, (i%4) || (i+3>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::SwappedDbl:
                itemData.ValueStr = formatDoubleValue(pointType, _lastData.value(i+3), _lastData.value(i+2), _lastData.value(i+1), value,
                                           byteOrder, (i%4) || (i+3>=rowCount()), itemData.Value);
            break;
                
            case DataDisplayMode::Int32:
                itemData.ValueStr = formatInt32Value(pointType, value, _lastData.value(i+1), byteOrder,
                                                    (i%2) || (i+1>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::SwappedInt32:
                itemData.ValueStr = formatInt32Value(pointType, _lastData.value(i+1), value, byteOrder,
                                                    (i%2) || (i+1>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::UInt32:
                itemData.ValueStr = formatUInt32Value(pointType, value, _lastData.value(i+1), byteOrder,
                                                            (i%2) || (i+1>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::SwappedUInt32:
                itemData.ValueStr = formatUInt32Value(pointType, _lastData.value(i+1), value, byteOrder,
                                                            (i%2) || (i+1>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::Int64:
                itemData.ValueStr = formatInt64Value(pointType, value, _lastData.value(i+1), _lastData.value(i+2), _lastData.value(i+3),
                                                     byteOrder, (i%4) || (i+3>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::SwappedInt64:
                itemData.ValueStr = formatInt64Value(pointType, _lastData.value(i+3), _lastData.value(i+2), _lastData.value(i+1), value,
                                                     byteOrder, (i%4) || (i+3>=rowCount()), itemData.Value);

            break;

            case DataDisplayMode::UInt64:
                itemData.ValueStr = formatUInt64Value(pointType, value, _lastData.value(i+1), _lastData.value(i+2), _lastData.value(i+3),
                                                      byteOrder, (i%4) || (i+3>=rowCount()), itemData.Value);
            break;

            case DataDisplayMode::SwappedUInt64:
                itemData.ValueStr = formatUInt64Value(pointType, _lastData.value(i+3), _lastData.value(i+2), _lastData.value(i+1), value,
                                                      byteOrder, (i%4) || (i+3>=rowCount()), itemData.Value);
            break;
        }
    }

    emit dataChanged(index(0), index(rowCount() - 1), QVector<int>() << Qt::DisplayRole);
}

///
/// \brief OutputListModel::find
/// \param deviceId
/// \param type
/// \param addr
/// \return
///
QModelIndex OutputListModel::find(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr) const
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
/// \brief OutputWidget::OutputWidget
/// \param parent
///
OutputWidget::OutputWidget(QWidget *parent) :
     QWidget(parent)
   , ui(new Ui::OutputWidget)
   ,_displayHexAddreses(false)
   ,_displayMode(DisplayMode::Data)
   ,_dataDisplayMode(DataDisplayMode::Hex)
   ,_byteOrder(ByteOrder::Direct)
   ,_listModel(new OutputListModel(this))
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
    ui->listView->setModel(_listModel.get());
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
}

///
/// \brief OutputWidget::~OutputWidget
///
OutputWidget::~OutputWidget()
{
    delete ui;
}

///
/// \brief OutputWidget::changeEvent
/// \param event
///
void OutputWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        if(!_listModel->isValid())
            setNotConnectedStatus();
    }

    QWidget::changeEvent(event);
}

///
/// \brief OutputWidget::data
/// \return
///
QVector<quint16> OutputWidget::data() const
{
    return _listModel->values();
}

///
/// \brief OutputWidget::setup
/// \param dd
/// \param simulations
/// \param data
///
void OutputWidget::setup(const DisplayDefinition& dd, const ModbusSimulationMap2& simulations, const QModbusDataUnit& data)
{
    _descriptionMap.insert(descriptionMap());
    _displayDefinition = dd;

    setLogViewLimit(dd.LogViewLimit);
    setAutosctollLogView(dd.AutoscrollLog);

    _listModel->clear();

    for(auto&& key : simulations.keys())
        _listModel->setData(_listModel->find(key.DeviceId, key.Type, key.Address), true, SimulationRole);

    for(auto&& key : _descriptionMap.keys())
        setDescription(key.DeviceId, key.Type, key.Address, _descriptionMap[key]);

    updateData(data);
}

///
/// \brief OutputWidget::displayHexAddresses
/// \return
///
bool OutputWidget::displayHexAddresses() const
{
    return _displayHexAddreses;
}

///
/// \brief OutputWidget::setDisplayHexAddresses
/// \param on
///
void OutputWidget::setDisplayHexAddresses(bool on)
{
    _displayHexAddreses = on;
    _listModel->update();
}

///
/// \brief OutputWidget::captureMode
/// \return
///
CaptureMode OutputWidget::captureMode() const
{
    return _fileCapture.isOpen() ? CaptureMode::TextCapture : CaptureMode::Off;
}

///
/// \brief OutputWidget::startTextCapture
/// \param file
///
void OutputWidget::startTextCapture(const QString& file)
{
    _fileCapture.setFileName(file);
    if(!_fileCapture.open(QFile::Text | QFile::WriteOnly))
        emit startTextCaptureError(_fileCapture.errorString());
}

///
/// \brief OutputWidget::stopTextCapture
///
void OutputWidget::stopTextCapture()
{
    if(_fileCapture.isOpen())
        _fileCapture.close();
}

///
/// \brief OutputWidget::backgroundColor
/// \return
///
QColor OutputWidget::backgroundColor() const
{
    return ui->listView->palette().color(QPalette::Base);
}

///
/// \brief OutputWidget::setBackgroundColor
/// \param clr
///
void OutputWidget::setBackgroundColor(const QColor& clr)
{
    auto pal = palette();
    pal.setColor(QPalette::Base, clr);
    pal.setColor(QPalette::Window, clr);
    setPalette(pal);
}

///
/// \brief OutputWidget::foregroundColor
/// \return
///
QColor OutputWidget::foregroundColor() const
{
    return ui->listView->palette().color(QPalette::Text);
}

///
/// \brief OutputWidget::setForegroundColor
/// \param clr
///
void OutputWidget::setForegroundColor(const QColor& clr)
{
    auto pal = ui->listView->palette();
    pal.setColor(QPalette::Text, clr);
    ui->listView->setPalette(pal);
}

///
/// \brief OutputWidget::statusColor
/// \return
///
QColor OutputWidget::statusColor() const
{
    return ui->labelStatus->palette().color(QPalette::WindowText);
}

///
/// \brief OutputWidget::setStatusColor
/// \param clr
///
void OutputWidget::setStatusColor(const QColor& clr)
{
    auto pal = ui->labelStatus->palette();
    pal.setColor(QPalette::WindowText, clr);
    ui->labelStatus->setPalette(pal);
    ui->modbusMsg->setStatusColor(clr);
}

///
/// \brief OutputWidget::font
/// \return
///
QFont OutputWidget::font() const
{
    return ui->listView->font();
}

///
/// \brief OutputWidget::setFont
/// \param font
///
void OutputWidget::setFont(const QFont& font)
{
    ui->listView->setFont(font);
    ui->labelStatus->setFont(font);
    ui->logView->setFont(font);
    ui->modbusMsg->setFont(font);
}

///
/// \brief OutputWidget::logViewLimit
/// \return
///
int OutputWidget::logViewLimit() const
{
    return ui->logView->rowLimit();
}

///
/// \brief OutputWidget::setLogViewLimit
/// \param l
///
void OutputWidget::setLogViewLimit(int l)
{
    ui->logView->setRowLimit(l);
}

///
/// \brief OutputWidget::pauseLogView
/// \param pause
///
void OutputWidget::setLogViewState(LogViewState state)
{
    ui->logView->setState(state);
}

///
/// \brief OutputWidget::autoscrollLogView
/// \return
///
bool OutputWidget::autoscrollLogView() const
{
    return ui->logView->autoscroll();
}

///
/// \brief OutputWidget::setAutosctollLogView
/// \param on
///
void OutputWidget::setAutosctollLogView(bool on)
{
    ui->logView->setAutoscroll(on);
}

///
/// \brief OutputWidget::clearLogView
///
void OutputWidget::clearLogView()
{
    ui->logView->clear();
    ui->modbusMsg->clear();
    hideModbusMessage();
}

///
/// \brief OutputWidget::setStatus
/// \param status
///
void OutputWidget::setStatus(const QString& status)
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
/// \brief OutputWidget::paint
/// \param rc
/// \param painter
///
void OutputWidget::paint(const QRect& rc, QPainter& painter)
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
        const auto text = _listModel->data(_listModel->index(i), Qt::DisplayRole).toString().trimmed();
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
/// \brief OutputWidget::updateTraffic
/// \param msg
///
void OutputWidget::updateTraffic(QSharedPointer<const ModbusMessage> msg)
{
    updateLogView(msg);
}

///
/// \brief OutputWidget::updateData
///
void OutputWidget::updateData(const QModbusDataUnit& data)
{
    _listModel->updateData(data);
}

///
/// \brief OutputWidget::descriptionMap
/// \return
///
AddressDescriptionMap2 OutputWidget::descriptionMap() const
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
/// \brief OutputWidget::setDescription
/// \param deviceId
/// \param type
/// \param addr
/// \param desc
///
void OutputWidget::setDescription(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, const QString& desc)
{
    _listModel->setData(_listModel->find(deviceId, type, addr), desc, DescriptionRole);
}

///
/// \brief OutputWidget::setSimulated
/// \param deviceId
/// \param type
/// \param addr
/// \param on
///
void OutputWidget::setSimulated(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 addr, bool on)
{
    _listModel->setData(_listModel->find(deviceId, type, addr), on, SimulationRole);
}

///
/// \brief OutputWidget::displayMode
/// \return
///
DisplayMode OutputWidget::displayMode() const
{
    return _displayMode;
}

///
/// \brief OutputWidget::setDisplayMode
/// \param mode
///
void OutputWidget::setDisplayMode(DisplayMode mode)
{
    _displayMode = mode;
    switch(mode)
    {
        case DisplayMode::Data:
            ui->stackedWidget->setCurrentIndex(0);
        break;

        case DisplayMode::Traffic:
            ui->stackedWidget->setCurrentIndex(1);
        break;

        default:
        break;
    }
}

///
/// \brief OutputWidget::dataDisplayMode
/// \return
///
DataDisplayMode OutputWidget::dataDisplayMode() const
{
    return _dataDisplayMode;
}

///
/// \brief OutputWidget::setDataDisplayMode
/// \param mode
///
void OutputWidget::setDataDisplayMode(DataDisplayMode mode)
{
    _dataDisplayMode = mode;
    ui->logView->setDataDisplayMode(mode);
    ui->modbusMsg->setDataDisplayMode(mode);

    _listModel->update();
}

///
/// \brief OutputWidget::byteOrder
/// \return
///
const ByteOrder* OutputWidget::byteOrder() const
{
    return &_byteOrder;
}

///
/// \brief OutputWidget::setByteOrder
/// \param order
///
void OutputWidget::setByteOrder(ByteOrder order)
{
    _byteOrder = order;
    ui->modbusMsg->setByteOrder(order);

    _listModel->update();
}

///
/// \brief OutputWidget::codepage
/// \return
///
QString OutputWidget::codepage() const
{
    return _codepage;
}

///
/// \brief OutputWidget::setCodepage
/// \param name
///
void OutputWidget::setCodepage(const QString& name)
{
    _codepage = name;
    _listModel->update();
}

///
/// \brief OutputWidget::on_listView_customContextMenuRequested
/// \param pos
///
void OutputWidget::on_listView_customContextMenuRequested(const QPoint &pos)
{
    const auto index = ui->listView->indexAt(pos);
    if(!index.isValid()) return;

    QInputDialog dlg(this);
    dlg.setLabelText(QString(tr("%1: Enter Description")).arg(_listModel->data(index, AddressStringRole).toString()));
    dlg.setTextValue(_listModel->data(index, DescriptionRole).toString());
    if(dlg.exec() == QDialog::Accepted)
    {
        _listModel->setData(index, dlg.textValue(), DescriptionRole);
    }
}

///
/// \brief OutputWidget::on_listView_doubleClicked
/// \param item
///
void OutputWidget::on_listView_doubleClicked(const QModelIndex& index)
{
    if(!index.isValid()) return;

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

    const auto address = _listModel->data(idx, AddressRole).toUInt();
    const auto value = _listModel->data(idx, ValueRole);

    emit itemDoubleClicked(address, value);
}

///
/// \brief OutputWidget::setNotConnectedStatus
///
void OutputWidget::setNotConnectedStatus()
{
    setStatus(tr("NOT CONNECTED!"));
}

///
/// \brief OutputWidget::setInvalidLengthStatus
///
void OutputWidget::setInvalidLengthStatus()
{
    setStatus(tr("Invalid Data Length Specified"));
}

///
/// \brief OutputWidget::captureString
/// \param s
///
void OutputWidget::captureString(const QString& s)
{
    if(_fileCapture.isOpen())
    {
        QTextStream stream(&_fileCapture);
        stream << s << "\n";
    }
}

///
/// \brief OutputWidget::showModbusMessage
/// \param index
///
void OutputWidget::showModbusMessage(const QModelIndex& index)
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
/// \brief OutputWidget::hideModbusMessage
///
void OutputWidget::hideModbusMessage()
{
    ui->splitter->setSizes({1, 0});
    ui->splitter->widget(1)->hide();
}

///
/// \brief OutputWidget::updateLogView
/// \param msg
///
void OutputWidget::updateLogView(QSharedPointer<const ModbusMessage> msg)
{
    ui->logView->addItem(msg);
    if(captureMode() == CaptureMode::TextCapture && msg != nullptr)
    {
        const auto str = QString("%1: %2 %3 %4").arg(
            (msg->isRequest()?  "Tx" : "Rx"),
            msg->timestamp().toString(Qt::ISODateWithMs),
            (msg->isRequest()?  "<<" : ">>"),
            msg->toString(DataDisplayMode::Hex));
        captureString(str);
    }
}
