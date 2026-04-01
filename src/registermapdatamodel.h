#ifndef REGISTERMAPDATAMODEL_H
#define REGISTERMAPDATAMODEL_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QMap>
#include <QList>
#include <QUuid>
#include <QDateTime>
#include <QModbusDataUnit>

#include "modbusmultiserver.h"
#include "controls/outputtypes.h"
#include "enums.h"

///
/// \brief Column indices for the register map table (shared between model and delegates)
///
enum RegisterMapColumn {
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
/// \brief Custom roles shared between RegisterMapDataModel and item delegates
///
namespace RegisterMapRole {
    constexpr int DeviceId  = Qt::UserRole;       // ColUnit: DeviceId as int
    constexpr int Type      = Qt::UserRole + 1;   // ColUnit: RegisterType as int
    constexpr int Address   = Qt::UserRole + 2;   // ColUnit: Address as int
    constexpr int TypeValue = Qt::UserRole + 3;   // ColType: RegisterType as int
    // Qt::UserRole on ColValue: raw quint16 value
}

///
/// \brief Register map entry data
///
struct RegisterMapEntry
{
    quint16       value   = 0;
    QString       comment;
    DataType      type    = DataType::Int16;
    RegisterOrder order   = RegisterOrder::MSRF;
    QDateTime     timestamp;
};

///
/// \brief The RegisterMapDataModel class — QAbstractTableModel backing the register map view
///
class RegisterMapDataModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit RegisterMapDataModel(ModbusMultiServer& server, QObject* parent = nullptr);
    ~RegisterMapDataModel() override;

    // QAbstractTableModel interface
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Data access
    ItemMapKey keyForRow(int row) const;
    const QMap<ItemMapKey, RegisterMapEntry>& entries() const;
    const QMap<ItemMapKey, QUuid>& uuids() const;
    bool contains(const ItemMapKey& key) const;
    bool isEmpty() const;
    ItemMapKey lastKey() const;

    // Modification
    void addEntry(const ItemMapKey& key, const RegisterMapEntry& entry);
    void removeEntries(QList<int> sourceRows);

    // External Modbus data update
    void applyMbDataChange(quint8 deviceId, const QModbusDataUnit& data);

    // Refresh address column display (call when ZeroBasedAddress preference changes)
    void refreshAddressColumn();

private:
    int  rowForKey(const ItemMapKey& key) const;
    void registerEntry(const ItemMapKey& key, const RegisterMapEntry& entry);
    void unregisterEntry(const ItemMapKey& key);

    ModbusMultiServer&               _server;
    QList<ItemMapKey>                _keys;
    QMap<ItemMapKey, RegisterMapEntry> _data;
    QMap<ItemMapKey, QUuid>          _uuids;
    bool                             _inSetData = false;
};

///
/// \brief The RegisterMapFilterProxy class — filters register map rows by Type and Unit
///
class RegisterMapFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit RegisterMapFilterProxy(QObject* parent = nullptr);

    void setFilterUnit(int unit);       // 0 = show all
    void setFilterTypeIndex(int index); // 0 = show all, 1-4 = Coils/Discrete/Input/Holding

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    int _filterUnit      = 0;
    int _filterTypeIndex = 0;
};

#endif // REGISTERMAPDATAMODEL_H
