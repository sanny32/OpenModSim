#ifndef DATAMAPDATAMODEL_H
#define DATAMAPDATAMODEL_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QColor>
#include <QMap>
#include <QList>
#include <QUuid>
#include <QDateTime>
#include <QModbusDataUnit>

#include "modbusmultiserver.h"
#include "controls/outputtypes.h"
#include "enums.h"

///
/// \brief Column indices for the DataMap table (shared between model and delegates)
///
enum DataMapColumn {
    ColUnit = 0,
    ColType,
    ColAddress,
    ColDataType,
    ColOrder,
    ColComment,
    ColValue,
    ColTimestamp,
    ColCount
};

///
/// \brief Custom roles shared between DataMapDataModel and item delegates
///
namespace DataMapRole {
    constexpr int DeviceId  = Qt::UserRole;       // ColUnit: DeviceId as int
    constexpr int Type      = Qt::UserRole + 1;   // ColUnit: RegisterType as int
    constexpr int Address   = Qt::UserRole + 2;   // ColUnit: Address as int
    constexpr int TypeValue = Qt::UserRole + 3;   // ColType: RegisterType as int
    constexpr int RowColor  = Qt::UserRole + 4;   // any column: row highlight color as QColor
    // Qt::UserRole on ColValue: raw quint16 value
}

///
/// \brief DataMap entry data
///
struct DataMapEntry
{
    quint16       value   = 0;
    QString       comment;
    DataType      type    = DataType::Int16;
    RegisterOrder order   = RegisterOrder::MSRF;
    QDateTime     timestamp;
    QColor        color;
};

///
/// \brief The DataMapDataModel class — QAbstractTableModel backing the DataMap view
///
class DataMapDataModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit DataMapDataModel(ModbusMultiServer& server, QObject* parent = nullptr);
    ~DataMapDataModel() override;

    // QAbstractTableModel interface
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                  const QModelIndex& destinationParent, int destinationChild) override;

    // Data access
    ItemMapKey keyForRow(int row) const;
    const QList<ItemMapKey>& keys() const { return _keys; }
    const QMap<ItemMapKey, DataMapEntry>& entries() const;
    const QMap<ItemMapKey, QUuid>& uuids() const;
    bool contains(const ItemMapKey& key) const;
    bool isEmpty() const;
    ItemMapKey lastKey() const;

    // Modification
    void addEntry(const ItemMapKey& key, const DataMapEntry& entry);
    void removeEntries(QList<int> sourceRows);

    // External Modbus data update
    void applyMbDataChange(quint8 deviceId, const QModbusDataUnit& data);
    void applyTimestampChange(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 address, const QDateTime& timestamp);
    void applyDescriptionChange(quint8 deviceId, QModbusDataUnit::RegisterType type, quint16 address, const QString& description);

    bool zeroBased() const { return _zeroBased; }
    bool hexView()   const { return _hexView; }
    void setZeroBased(bool v);
    void setHexView(bool v);

    // Refresh Unit and Address column display
    void refreshUnitAndAddressColumns();

private:
    int  rowForKey(const ItemMapKey& key) const;
    bool hasAnyRowWithKey(const ItemMapKey& key, int exceptRow = -1) const;
    void registerEntry(const ItemMapKey& key, const DataMapEntry& entry);
    void unregisterEntry(const ItemMapKey& key);

    ModbusMultiServer&               _server;
    QList<ItemMapKey>                _keys;
    QMap<ItemMapKey, DataMapEntry> _data;
    QMap<ItemMapKey, QUuid>          _uuids;
    bool                             _inSetData  = false;
    bool                             _zeroBased  = false;
    bool                             _hexView    = false;
};

///
/// \brief The DataMapFilterProxy class — filters DataMap rows by Type and Unit
///
class DataMapFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit DataMapFilterProxy(QObject* parent = nullptr);

    void setFilterUnit(int unit);       // 0 = show all
    void setFilterTypeIndex(int index); // 0 = show all, 1-4 = Coils/Discrete/Input/Holding

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    int _filterUnit      = 0;
    int _filterTypeIndex = 0;
};

#endif // DATAMAPDATAMODEL_H


