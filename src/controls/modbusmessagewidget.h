#ifndef MODBUSMESSAGEWIDGET_H
#define MODBUSMESSAGEWIDGET_H

#include <QListWidget>
#include "modbusmessage.h"

///
/// \brief The ModbusMessageWidget class
///
class ModbusMessageWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit ModbusMessageWidget(QWidget *parent = nullptr);

    void clear();

    DataType dataType() const;
    void setDataType(DataType type);

    ByteOrder byteOrder() const;
    void setByteOrder(ByteOrder order);

    QSharedPointer<const ModbusMessage> modbusMessage() const;
    void setModbusMessage(QSharedPointer<const ModbusMessage> msg);

    bool showTimestamp() const;
    void setShowTimestamp(bool on);

    bool showLeadingZeros() const;
    void setShowLeadingZeros(bool value);

    QColor backgroundColor() const;
    void setBackGroundColor(const QColor& clr);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_customContextMenuRequested(const QPoint &pos);

private:
    void update();

private:
    QColor _statusClr;
    ByteOrder _byteOrder;
    DataType _dataType;
    bool _showLeadingZeros;
    bool _showTimestamp;
    QAction* _copyAct;
    QAction* _copyValuesAct;
    QSharedPointer<const ModbusMessage> _mm;
};

#endif // MODBUSMESSAGEWIDGET_H
