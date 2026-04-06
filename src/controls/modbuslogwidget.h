#ifndef MODBUSLOGWIDGET_H
#define MODBUSLOGWIDGET_H

#include <QQueue>
#include <QVector>
#include <QListView>
#include "bufferinglistmodel.h"
#include "modbusmessage.h"

class ModbusLogWidget;

///
/// \brief The ModbusLogModel class
///
class  ModbusLogModel : public BufferingListModel<QSharedPointer<const ModbusMessage>>
{
    Q_OBJECT

public:
    explicit ModbusLogModel(ModbusLogWidget* parent);
    QVariant data(const QModelIndex& index, int role) const override;

private:
    ModbusLogWidget* _parentWidget;
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
    void addItems(const QVector<QSharedPointer<const ModbusMessage>>& messages);
    QSharedPointer<const ModbusMessage> itemAt(const QModelIndex& index);
    bool exportToTextFile(const QString& filePath);

    DataType dataType() const;
    void setDataType(DataType type);

    bool showLeadingZeros() const;
    void setShowLeadingZeros(bool value);

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
    DataType _dataType;
    bool _showLeadingZeros = true;
};

#endif // MODBUSLOGWIDGET_H

