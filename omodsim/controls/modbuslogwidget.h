#ifndef MODBUSLOGWIDGET_H
#define MODBUSLOGWIDGET_H

#include <QQueue>
#include <QListView>
#include "modbusmessage.h"

class ModbusLogWidget;

///
/// \brief The ModbusLogModel class
///
class  ModbusLogModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ModbusLogModel(ModbusLogWidget* parent);
    ~ModbusLogModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    void clear();
    void append(QSharedPointer<const ModbusMessage> data);
    void update(){
        emit dataChanged(index(0), index(_items.size() - 1));
    }

    int rowLimit() const;
    void setRowLimit(int val);

    bool isBufferingMode() const {
        return _bufferingMode;
    }
    void setBufferingMode(bool value);

private:
    void deleteItems();

private:
    int _rowLimit = 30;
    bool _bufferingMode = false;
    ModbusLogWidget* _parentWidget;
    QQueue<QSharedPointer<const ModbusMessage>> _items;
    QQueue<QSharedPointer<const ModbusMessage>> _bufferingItems;
};

///
/// \brief The ModbusLogWidget class
///
class ModbusLogWidget : public QListView
{
    Q_OBJECT
public:
    explicit ModbusLogWidget(QWidget* parent = nullptr);

    void clear();

    int rowCount() const;
    QModelIndex index(int row);

    void addItem(QSharedPointer<const ModbusMessage> msg);
    QSharedPointer<const ModbusMessage> itemAt(const QModelIndex& index);

    DataDisplayMode dataDisplayMode() const;
    void setDataDisplayMode(DataDisplayMode mode);

    int rowLimit() const;
    void setRowLimit(int val);

    bool autoscroll() const;
    void setAutoscroll(bool on);

    QColor backgroundColor() const;
    void setBackGroundColor(const QColor& clr);

    LogViewState state() const;
    void setState(LogViewState state);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_customContextMenuRequested(const QPoint &pos);

private:
    bool _autoscroll;
    QAction* _copyAct;
    QAction* _copyBytesAct;
    LogViewState _state = LogViewState::Running;
    DataDisplayMode _dataDisplayMode;
};

#endif // MODBUSLOGWIDGET_H
