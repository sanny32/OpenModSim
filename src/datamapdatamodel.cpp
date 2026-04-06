#include <QtWidgets>
#include <QMimeData>
#include <QDataStream>
#include "datamapdatamodel.h"
#include "formatutils.h"
#include "numericutils.h"

namespace {

///
/// \brief registerTypeToString
///
QString registerTypeToString(QModbusDataUnit::RegisterType type)
{
    switch (type) {
        case QModbusDataUnit::Coils:            return QObject::tr("Coils");
        case QModbusDataUnit::DiscreteInputs:   return QObject::tr("Discrete Inputs");
        case QModbusDataUnit::InputRegisters:   return QObject::tr("Input Registers");
        case QModbusDataUnit::HoldingRegisters: return QObject::tr("Holding Registers");
        default:                                return {};
    }
}

///
/// \brief stringToRegisterType
///
QModbusDataUnit::RegisterType stringToRegisterType(const QString& str)
{
    if (str == QObject::tr("Coils"))           return QModbusDataUnit::Coils;
    if (str == QObject::tr("Discrete Inputs")) return QModbusDataUnit::DiscreteInputs;
    if (str == QObject::tr("Input Registers")) return QModbusDataUnit::InputRegisters;
    return QModbusDataUnit::HoldingRegisters;
}

///
/// \brief addressToDisplay
///
QString addressToDisplay(quint16 addr, bool zeroBased)
{
    return QString::number(zeroBased ? addr : static_cast<quint32>(addr) + 1);
}

///
/// \brief addressFromDisplay
///
quint16 addressFromDisplay(const QString& text, bool zeroBased, bool* ok = nullptr)
{
    bool localOk = false;
    const quint32 v = text.toUInt(&localOk);
    if (ok) *ok = localOk;
    if (!localOk) return 0;

    if (zeroBased)
        return static_cast<quint16>(qMin(v, static_cast<quint32>(0xFFFF)));

    return static_cast<quint16>(v > 0 ? qMin(v - 1, static_cast<quint32>(0xFFFF)) : 0);
}

///
/// \brief regsForKey
///
QVector<quint16> regsForKey(ModbusMultiServer& server, const ItemMapKey& key, DataType type)
{
    const int count = registersCount(type);
    const QModbusDataUnit unit = server.data(key.DeviceId, key.Type, key.Address, count);
    QVector<quint16> regs;
    for (int i = 0; i < count; ++i)
        regs << (unit.isValid() ? static_cast<quint16>(unit.value(i)) : 0);
    return regs;
}

///
/// \brief parseLocalizedFloat
///
bool parseLocalizedFloat(const QString& text, float& value)
{
    bool ok = false;
    value = QLocale().toFloat(text, &ok);
    if (!ok)
        value = QLocale::c().toFloat(text, &ok);
    return ok;
}

///
/// \brief parseLocalizedDouble
///
bool parseLocalizedDouble(const QString& text, double& value)
{
    bool ok = false;
    value = QLocale().toDouble(text, &ok);
    if (!ok)
        value = QLocale::c().toDouble(text, &ok);
    return ok;
}

///
/// \brief formatValue
///
QString formatValue(QModbusDataUnit::RegisterType regType,
                    DataType type, RegisterOrder order, const QVector<quint16>& regs)
{
    if (regs.isEmpty()) return {};
    QVariant outValue;
    switch (type) {
        case DataType::Binary:
            return formatBinaryValue(regType, regs[0], ByteOrder::Direct, outValue, false);
        case DataType::UInt16:
            return formatUInt16Value(regType, regs[0], ByteOrder::Direct, false, outValue, false);
        case DataType::Int16:
            return formatInt16Value(regType, static_cast<qint16>(regs[0]), ByteOrder::Direct, outValue, false);
        case DataType::Ansi:
            return formatAnsiValue(regType, regs[0], ByteOrder::Direct, QString(), outValue, false);
        case DataType::Float32:
            if (regs.size() >= 2) {
                const QVariant val = makeValue(regs, type, order, ByteOrder::Direct);
                if (val.isValid()) return QLocale().toString(val.toFloat(), 'g');
            }
            return formatHexValue(regType, regs[0], ByteOrder::Direct, outValue, false);
        case DataType::Float64:
            if (regs.size() >= 4) {
                const QVariant val = makeValue(regs, type, order, ByteOrder::Direct);
                if (val.isValid()) return QLocale().toString(val.toDouble(), 'g');
            }
            return formatHexValue(regType, regs[0], ByteOrder::Direct, outValue, false);
        default:
            if (isMultiRegisterType(type) && regs.size() >= registersCount(type)) {
                const QVariant val = makeValue(regs, type, order, ByteOrder::Direct);
                if (val.isValid()) return val.toString();
            }
            return formatHexValue(regType, regs[0], ByteOrder::Direct, outValue, false);
    }
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// DataMapDataModel
// ─────────────────────────────────────────────────────────────────────────────

///
/// \brief DataMapDataModel::DataMapDataModel
///
DataMapDataModel::DataMapDataModel(ModbusMultiServer& server, QObject* parent)
    : QAbstractTableModel(parent)
    , _server(server)
{
}

///
/// \brief DataMapDataModel::~DataMapDataModel
///
DataMapDataModel::~DataMapDataModel()
{
    for (const ItemMapKey& key : _keys)
        unregisterEntry(key);
}

///
/// \brief DataMapDataModel::rowCount
///
int DataMapDataModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : _keys.size();
}

///
/// \brief DataMapDataModel::columnCount
///
int DataMapDataModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

///
/// \brief DataMapDataModel::data
///
QVariant DataMapDataModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return {};
    const int row = index.row();
    const int col = index.column();
    if (row < 0 || row >= _keys.size()) return {};

    const ItemMapKey& key   = _keys[row];
    const DataMapEntry& e = _data[key];

    if (role == Qt::TextAlignmentRole) {
        if (col == ColComment) return static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft);
        if (col == ColAddress || col == ColValue) return static_cast<int>(Qt::AlignVCenter | Qt::AlignRight);
        return static_cast<int>(Qt::AlignCenter);
    }

    switch (col) {
        case ColUnit:
            if (role == Qt::DisplayRole || role == Qt::EditRole)
                return QString::number(key.DeviceId);
            if (role == DataMapRole::DeviceId) return static_cast<int>(key.DeviceId);
            if (role == DataMapRole::Type)     return static_cast<int>(key.Type);
            if (role == DataMapRole::Address)  return static_cast<int>(key.Address);
            break;

        case ColType:
            if (role == Qt::DisplayRole || role == Qt::EditRole)
                return registerTypeToString(key.Type);
            if (role == DataMapRole::TypeValue) return static_cast<int>(key.Type);
            break;

        case ColAddress:
            if (role == Qt::DisplayRole || role == Qt::EditRole)
                return addressToDisplay(key.Address, _zeroBased);
            break;

        case ColDataType:
            if (role == Qt::DisplayRole) {
                switch (e.type) {
                    case DataType::Binary:  return tr("Binary");
                    case DataType::UInt16:  return tr("UInt16");
                    case DataType::Int16:   return tr("Int16");
                    case DataType::Hex:     return tr("Hex");
                    case DataType::Float32: return tr("Float32");
                    case DataType::Float64: return tr("Float64");
                    case DataType::Int32:   return tr("Int32");
                    case DataType::UInt32:  return tr("UInt32");
                    case DataType::Int64:   return tr("Int64");
                    case DataType::UInt64:  return tr("UInt64");
                    case DataType::Ansi:    return tr("Ansi");
                }
            }
            if (role == Qt::EditRole)
                return enumToString(e.type);
            break;

        case ColOrder:
            if (role == Qt::DisplayRole) {
                if (!isMultiRegisterType(e.type)) return QString();
                return (e.order == RegisterOrder::MSRF) ? tr("MSRF") : tr("LSRF");
            }
            if (role == Qt::EditRole)
                return isMultiRegisterType(e.type) ? enumToString(e.order) : QString();
            break;

        case ColComment:
            if (role == Qt::DisplayRole || role == Qt::EditRole)
                return e.comment;
            break;

        case ColValue:
            if (role == Qt::DisplayRole)
                return formatValue(key.Type, e.type, e.order, regsForKey(_server, key, e.type));
            if (role == Qt::EditRole) {
                const auto regs = regsForKey(_server, key, e.type);
                if ((e.type == DataType::Float32 || e.type == DataType::Float64) &&
                    regs.size() >= registersCount(e.type))
                    return makeValue(regs, e.type, e.order, ByteOrder::Direct);
                return formatValue(key.Type, e.type, e.order, regs);
            }
            if (role == Qt::UserRole)
                return static_cast<quint32>(e.value);
            break;

        case ColTimestamp:
            if (role == Qt::DisplayRole)
                return e.timestamp.isValid() ? e.timestamp.toString(Qt::ISODate) : QString();
            break;

        default:
            break;
    }

    return {};
}

///
/// \brief DataMapDataModel::setData
///
bool DataMapDataModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || _inSetData) return false;
    const int row = index.row();
    const int col = index.column();
    if (row < 0 || row >= _keys.size()) return false;

    _inSetData = true;
    const ItemMapKey oldKey = _keys[row];
    DataMapEntry entry  = _data[oldKey];
    bool handled = false;

    // ── Value column ──────────────────────────────────────────────────────────
    if (col == ColValue && (role == Qt::EditRole || role == Qt::DisplayRole)) {
        const QString text = value.toString().trimmed();
        const int regCount = registersCount(entry.type);
        const bool lsrf    = (entry.order == RegisterOrder::LSRF);
        QVector<quint16> regs(regCount, 0);
        bool ok = false;

        switch (entry.type) {
            case DataType::Float32: {
                float f = 0.0f;
                ok = parseLocalizedFloat(text, f);
                if (ok) {
                    if (lsrf) breakFloat(f, regs[0], regs[1], ByteOrder::Direct);
                    else       breakFloat(f, regs[1], regs[0], ByteOrder::Direct);
                }
                break;
            }
            case DataType::Float64: {
                double d = 0.0;
                ok = parseLocalizedDouble(text, d);
                if (ok) {
                    if (lsrf) breakDouble(d, regs[0], regs[1], regs[2], regs[3], ByteOrder::Direct);
                    else       breakDouble(d, regs[3], regs[2], regs[1], regs[0], ByteOrder::Direct);
                }
                break;
            }
            case DataType::Int32: {
                const qint32 v = static_cast<qint32>(text.toLong(&ok));
                if (ok) {
                    if (lsrf) breakInt32(v, regs[0], regs[1], ByteOrder::Direct);
                    else       breakInt32(v, regs[1], regs[0], ByteOrder::Direct);
                }
                break;
            }
            case DataType::UInt32: {
                const quint32 v = static_cast<quint32>(text.toULong(&ok));
                if (ok) {
                    if (lsrf) breakUInt32(v, regs[0], regs[1], ByteOrder::Direct);
                    else       breakUInt32(v, regs[1], regs[0], ByteOrder::Direct);
                }
                break;
            }
            case DataType::Int64: {
                const qint64 v = text.toLongLong(&ok);
                if (ok) {
                    if (lsrf) breakInt64(v, regs[0], regs[1], regs[2], regs[3], ByteOrder::Direct);
                    else       breakInt64(v, regs[3], regs[2], regs[1], regs[0], ByteOrder::Direct);
                }
                break;
            }
            case DataType::UInt64: {
                const quint64 v = text.toULongLong(&ok);
                if (ok) {
                    if (lsrf) breakUInt64(v, regs[0], regs[1], regs[2], regs[3], ByteOrder::Direct);
                    else       breakUInt64(v, regs[3], regs[2], regs[1], regs[0], ByteOrder::Direct);
                }
                break;
            }
            default: {
                quint16 newValue = 0;
                if (text.startsWith("0x") || text.startsWith("0X"))
                    newValue = static_cast<quint16>(text.toUInt(&ok, 16));
                else if (entry.type == DataType::Int16)
                    newValue = static_cast<quint16>(text.toShort(&ok));
                else
                    newValue = text.toUShort(&ok);
                if (ok) regs[0] = newValue;
                break;
            }
        }

        if (ok) {
            QModbusDataUnit unit(oldKey.Type, oldKey.Address, regCount);
            for (int i = 0; i < regCount; ++i) unit.setValue(i, regs[i]);
            _server.setData(oldKey.DeviceId, unit);

            entry.value     = regs[0];
            entry.timestamp = _server.timestamp(oldKey.DeviceId, oldKey.Type, oldKey.Address);
            _data[oldKey]   = entry;
        }
        // Always refresh the cell (restores previous value if parse failed)
        emit dataChanged(createIndex(row, ColValue), createIndex(row, ColTimestamp));
        handled = true;
    }

    // ── Comment column ────────────────────────────────────────────────────────
    else if (col == ColComment && (role == Qt::EditRole || role == Qt::DisplayRole)) {
        entry.comment  = value.toString();
        _data[oldKey]  = entry;
        _server.setDescription(oldKey.DeviceId, oldKey.Type, oldKey.Address, entry.comment);
        emit dataChanged(index, index);
        handled = true;
    }

    // ── DataType column ───────────────────────────────────────────────────────
    else if (col == ColDataType && (role == Qt::EditRole || role == Qt::DisplayRole)) {
        entry.type = enumFromString<DataType>(value.toString(), DataType::Int16);
        if (!isMultiRegisterType(entry.type))
            entry.order = RegisterOrder::MSRF;
        _data[oldKey] = entry;
        registerEntry(oldKey, entry); // register count may have changed
        emit dataChanged(createIndex(row, ColDataType), createIndex(row, ColValue));
        handled = true;
    }

    // ── Order column ──────────────────────────────────────────────────────────
    else if (col == ColOrder && (role == Qt::EditRole || role == Qt::DisplayRole)) {
        entry.order   = enumFromString<RegisterOrder>(value.toString(), RegisterOrder::MSRF);
        _data[oldKey] = entry;
        emit dataChanged(createIndex(row, ColOrder), createIndex(row, ColValue));
        handled = true;
    }

    // ── Key columns: Unit / Type / Address ────────────────────────────────────
    else if (col == ColUnit || col == ColType || col == ColAddress) {
        ItemMapKey newKey = oldKey;

        if (col == ColUnit) {
            newKey.DeviceId = static_cast<quint8>(value.toString().toUShort());
        } else if (col == ColType) {
            if (role == DataMapRole::TypeValue)
                newKey.Type = static_cast<QModbusDataUnit::RegisterType>(value.toInt());
            else
                newKey.Type = stringToRegisterType(value.toString());
        } else {
            newKey.Address = addressFromDisplay(value.toString(), _zeroBased);
        }

        if (newKey.DeviceId == oldKey.DeviceId &&
            newKey.Type    == oldKey.Type    &&
            newKey.Address == oldKey.Address) {
            // No actual change — still refresh display (in case role was redundant write)
            emit dataChanged(createIndex(row, ColUnit), createIndex(row, ColTimestamp));
            handled = true;
        } else {
            unregisterEntry(oldKey);
            _data.remove(oldKey);

            const bool bitType = (newKey.Type == QModbusDataUnit::Coils ||
                                  newKey.Type == QModbusDataUnit::DiscreteInputs);
            if (bitType) {
                entry.type  = DataType::Binary;
                entry.order = RegisterOrder::MSRF;
            }

            const auto unit   = _server.data(newKey.DeviceId, newKey.Type, newKey.Address, 1);
            entry.value       = unit.isValid() ? static_cast<quint16>(unit.value(0)) : 0;
            entry.timestamp   = _server.timestamp(newKey.DeviceId, newKey.Type, newKey.Address);
            entry.comment     = _server.description(newKey.DeviceId, newKey.Type, newKey.Address);

            _keys[row] = newKey;
            _data[newKey] = entry;
            registerEntry(newKey, entry);

            // Timestamp for a new key/type can become available only after
            // registerEntry() has updated backend unit maps.
            if (!_data[newKey].timestamp.isValid()) {
                const auto backendTsAfterRegister = _server.timestamp(newKey.DeviceId, newKey.Type, newKey.Address);
                if (backendTsAfterRegister.isValid())
                    _data[newKey].timestamp = backendTsAfterRegister;
            }

            emit dataChanged(createIndex(row, ColUnit), createIndex(row, ColTimestamp));
            handled = true;
        }
    }

    _inSetData = false;
    return handled;
}

///
/// \brief DataMapDataModel::headerData
///
QVariant DataMapDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
        case ColUnit:      return tr("Unit");
        case ColType:      return tr("Type");
        case ColAddress:   return tr("Address");
        case ColDataType:  return tr("Data Type");
        case ColOrder:     return tr("Order");
        case ColComment:   return tr("Comment");
        case ColValue:     return tr("Value");
        case ColTimestamp: return tr("Timestamp");
        default:           return {};
    }
}

///
/// \brief DataMapDataModel::flags
///
Qt::ItemFlags DataMapDataModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    if (!index.isValid())
        return f | Qt::ItemIsDropEnabled;

    f |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

    if (index.column() == ColTimestamp)
        return f; // read-only

    f |= Qt::ItemIsEditable;

    if (index.column() == ColOrder) {
        const ItemMapKey& key = _keys[index.row()];
        if (!isMultiRegisterType(_data[key].type))
            f &= ~Qt::ItemIsEditable;
    }

    return f;
}

///
/// \brief DataMapDataModel::supportedDropActions
///
Qt::DropActions DataMapDataModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

///
/// \brief DataMapDataModel::mimeTypes
///
QStringList DataMapDataModel::mimeTypes() const
{
    return { QStringLiteral("application/x-omodsim-DataMap-row") };
}

///
/// \brief DataMapDataModel::mimeData
///
QMimeData* DataMapDataModel::mimeData(const QModelIndexList& indexes) const
{
    if (indexes.isEmpty()) return nullptr;
    auto* data = new QMimeData;
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    stream << indexes.first().row();
    data->setData(QStringLiteral("application/x-omodsim-DataMap-row"), encoded);
    return data;
}

///
/// \brief DataMapDataModel::canDropMimeData
///
bool DataMapDataModel::canDropMimeData(const QMimeData* data, Qt::DropAction action,
                                            int row, int column, const QModelIndex& parent) const
{
    Q_UNUSED(row); Q_UNUSED(column); Q_UNUSED(parent);
    return action == Qt::MoveAction &&
           data->hasFormat(QStringLiteral("application/x-omodsim-DataMap-row"));
}

///
/// \brief DataMapDataModel::dropMimeData
///
bool DataMapDataModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
                                         int row, int column, const QModelIndex& parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    QByteArray encoded = data->data(QStringLiteral("application/x-omodsim-DataMap-row"));
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    int fromRow;
    stream >> fromRow;

    const int toRow = (row == -1) ? rowCount() : row;
    return moveRows({}, fromRow, 1, {}, toRow);
}

///
/// \brief DataMapDataModel::moveRows
///
bool DataMapDataModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                                     const QModelIndex& destinationParent, int destinationChild)
{
    if (sourceParent.isValid() || destinationParent.isValid() || count != 1)
        return false;
    if (sourceRow < 0 || sourceRow >= _keys.size())
        return false;
    if (destinationChild < 0 || destinationChild > _keys.size())
        return false;
    if (sourceRow == destinationChild || sourceRow == destinationChild - 1)
        return false;

    if (!beginMoveRows({}, sourceRow, sourceRow, {}, destinationChild))
        return false;

    const ItemMapKey key = _keys.takeAt(sourceRow);
    const int insertPos = (destinationChild > sourceRow) ? destinationChild - 1 : destinationChild;
    _keys.insert(insertPos, key);

    endMoveRows();
    return true;
}

///
/// \brief DataMapDataModel::keyForRow
///
ItemMapKey DataMapDataModel::keyForRow(int row) const
{
    if (row < 0 || row >= _keys.size()) return { 0, QModbusDataUnit::HoldingRegisters, 0 };
    return _keys[row];
}

///
/// \brief DataMapDataModel::entries
///
const QMap<ItemMapKey, DataMapEntry>& DataMapDataModel::entries() const
{
    return _data;
}

///
/// \brief DataMapDataModel::uuids
///
const QMap<ItemMapKey, QUuid>& DataMapDataModel::uuids() const
{
    return _uuids;
}

///
/// \brief DataMapDataModel::contains
///
bool DataMapDataModel::contains(const ItemMapKey& key) const
{
    return _data.contains(key);
}

///
/// \brief DataMapDataModel::isEmpty
///
bool DataMapDataModel::isEmpty() const
{
    return _keys.isEmpty();
}

///
/// \brief DataMapDataModel::lastKey
///
ItemMapKey DataMapDataModel::lastKey() const
{
    if (_keys.isEmpty()) return { 1, QModbusDataUnit::HoldingRegisters, 0 };
    return _keys.last();
}

///
/// \brief DataMapDataModel::addEntry
///
void DataMapDataModel::addEntry(const ItemMapKey& key, const DataMapEntry& entry)
{
    DataMapEntry normalizedEntry = entry;
    const auto backendTs = _server.timestamp(key.DeviceId, key.Type, key.Address);
    if (backendTs.isValid()) {
        normalizedEntry.timestamp = backendTs;
    } else if (normalizedEntry.timestamp.isValid()) {
        _server.setTimestamp(key.DeviceId, key.Type, key.Address, normalizedEntry.timestamp);
    }

    const auto backendDesc = _server.description(key.DeviceId, key.Type, key.Address);
    if (!backendDesc.isEmpty()) {
        normalizedEntry.comment = backendDesc;
    } else if (!normalizedEntry.comment.isEmpty()) {
        _server.setDescription(key.DeviceId, key.Type, key.Address, normalizedEntry.comment);
    }

    if (_data.contains(key)) {
        // Update existing entry in place
        const int row = rowForKey(key);
        _data[key] = normalizedEntry;
        if (row >= 0)
            emit dataChanged(createIndex(row, 0), createIndex(row, ColCount - 1));
        return;
    }

    const int row = _keys.size();
    beginInsertRows({}, row, row);
    _keys.append(key);
    _data[key] = normalizedEntry;
    endInsertRows();

    registerEntry(key, normalizedEntry);

    // For freshly added keys, backend timestamp can become available only after
    // the unit map is registered in registerEntry().
    if (!normalizedEntry.timestamp.isValid()) {
        const auto backendTsAfterRegister = _server.timestamp(key.DeviceId, key.Type, key.Address);
        if (backendTsAfterRegister.isValid()) {
            _data[key].timestamp = backendTsAfterRegister;
            const int insertedRow = rowForKey(key);
            if (insertedRow >= 0)
                emit dataChanged(createIndex(insertedRow, ColTimestamp), createIndex(insertedRow, ColTimestamp));
        }
    }
}

///
/// \brief DataMapDataModel::removeEntries
///
void DataMapDataModel::removeEntries(QList<int> sourceRows)
{
    // Sort descending so removing by index doesn't shift subsequent indices
    std::sort(sourceRows.begin(), sourceRows.end(), std::greater<int>());
    for (int row : sourceRows) {
        if (row < 0 || row >= _keys.size()) continue;
        const ItemMapKey key = _keys[row];
        unregisterEntry(key);
        beginRemoveRows({}, row, row);
        _keys.removeAt(row);
        _data.remove(key);
        endRemoveRows();
    }
}

///
/// \brief DataMapDataModel::applyMbDataChange
///
void DataMapDataModel::applyMbDataChange(quint8 deviceId, const QModbusDataUnit& data)
{
    if (!data.isValid() || _inSetData) return;

    for (quint32 i = 0; i < data.valueCount(); ++i) {
        const ItemMapKey key{ deviceId, data.registerType(),
                              static_cast<quint16>(data.startAddress() + i) };
        const quint16 value = static_cast<quint16>(data.value(i));

        auto it = _data.find(key);
        if (it == _data.end()) continue;
        if (it->value == value) continue;

        it->value     = value;
        it->timestamp = _server.timestamp(deviceId, key.Type, key.Address);

        const int row = rowForKey(key);
        if (row >= 0)
            emit dataChanged(createIndex(row, ColValue), createIndex(row, ColTimestamp));
    }
}

///
/// \brief DataMapDataModel::applyTimestampChange
/// Applies backend timestamp update to existing register-map rows.
///
void DataMapDataModel::applyTimestampChange(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 address, const QDateTime& timestamp)
{
    const ItemMapKey key{deviceId, type, address};
    auto it = _data.find(key);
    if (it == _data.end())
        return;

    if (it->timestamp == timestamp)
        return;

    it->timestamp = timestamp;
    const int row = rowForKey(key);
    if (row >= 0)
        emit dataChanged(createIndex(row, ColTimestamp), createIndex(row, ColTimestamp));
}

///
/// \brief DataMapDataModel::applyDescriptionChange
/// Applies backend description update to existing register-map rows.
///
void DataMapDataModel::applyDescriptionChange(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 address, const QString& description)
{
    const ItemMapKey key{deviceId, type, address};
    auto it = _data.find(key);
    if (it == _data.end())
        return;

    if (it->comment == description)
        return;

    it->comment = description;
    const int row = rowForKey(key);
    if (row >= 0)
        emit dataChanged(createIndex(row, ColComment), createIndex(row, ColComment));
}

///
/// \brief DataMapDataModel::setZeroBased
///
void DataMapDataModel::setZeroBased(bool v)
{
    if (_zeroBased == v) return;
    _zeroBased = v;
    refreshUnitAndAddressColumns();
}

///
/// \brief DataMapDataModel::setHexView
///
void DataMapDataModel::setHexView(bool v)
{
    if (_hexView == v) return;
    _hexView = v;
    refreshUnitAndAddressColumns();
}

///
/// \brief DataMapDataModel::refreshUnitAndAddressColumns
///
void DataMapDataModel::refreshUnitAndAddressColumns()
{
    if (_keys.isEmpty()) return;
    emit dataChanged(createIndex(0, ColUnit),
                     createIndex(_keys.size() - 1, ColAddress));
}

///
/// \brief DataMapDataModel::rowForKey
///
int DataMapDataModel::rowForKey(const ItemMapKey& key) const
{
    for (int i = 0; i < _keys.size(); ++i) {
        const ItemMapKey& k = _keys[i];
        if (k.DeviceId == key.DeviceId && k.Type == key.Type && k.Address == key.Address)
            return i;
    }
    return -1;
}

///
/// \brief DataMapDataModel::registerEntry
///
void DataMapDataModel::registerEntry(const ItemMapKey& key, const DataMapEntry& entry)
{
    if (!_uuids.contains(key))
        _uuids[key] = QUuid::createUuid();
    _server.addDeviceId(key.DeviceId);
    _server.addUnitMap(_uuids[key], key.DeviceId, key.Type,
                       key.Address, registersCount(entry.type));
}

///
/// \brief DataMapDataModel::unregisterEntry
///
void DataMapDataModel::unregisterEntry(const ItemMapKey& key)
{
    const QUuid uuid = _uuids.take(key);
    if (uuid.isNull()) return;
    _server.removeUnitMap(uuid, key.DeviceId);
    _server.removeDeviceId(key.DeviceId);
}

// ─────────────────────────────────────────────────────────────────────────────
// DataMapFilterProxy
// ─────────────────────────────────────────────────────────────────────────────

///
/// \brief DataMapFilterProxy::DataMapFilterProxy
///
DataMapFilterProxy::DataMapFilterProxy(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

///
/// \brief DataMapFilterProxy::setFilterUnit
///
void DataMapFilterProxy::setFilterUnit(int unit)
{
    if (_filterUnit == unit) return;
    _filterUnit = unit;
    invalidateFilter();
}

///
/// \brief DataMapFilterProxy::setFilterTypeIndex
///
void DataMapFilterProxy::setFilterTypeIndex(int index)
{
    if (_filterTypeIndex == index) return;
    _filterTypeIndex = index;
    invalidateFilter();
}

///
/// \brief DataMapFilterProxy::filterAcceptsRow
///
bool DataMapFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex& /*sourceParent*/) const
{
    const auto* src = static_cast<const DataMapDataModel*>(sourceModel());
    if (!src) return true;

    const ItemMapKey key = src->keyForRow(sourceRow);

    if (_filterUnit != 0 && key.DeviceId != static_cast<quint8>(_filterUnit))
        return false;

    if (_filterTypeIndex != 0) {
        static const QModbusDataUnit::RegisterType types[] = {
            QModbusDataUnit::Coils,
            QModbusDataUnit::DiscreteInputs,
            QModbusDataUnit::InputRegisters,
            QModbusDataUnit::HoldingRegisters
        };
        if (key.Type != types[_filterTypeIndex - 1])
            return false;
    }

    return true;
}


