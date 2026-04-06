#ifndef TRAFFICLOGWINDOW_H
#define TRAFFICLOGWINDOW_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QToolButton>
#include "modbusmultiserver.h"
#include "modbuslogwidget.h"

///
/// \brief The TrafficLogWindow class
/// Global Modbus traffic log with filtering (replaces per-form traffic tab and DialogRawDataLog).
///
class TrafficLogWindow : public QWidget
{
    Q_OBJECT
public:
    explicit TrafficLogWindow(ModbusMultiServer& server, QWidget* parent = nullptr);

    DataType dataType() const;
    void setDataType(DataType type);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_mbRequest(QSharedPointer<const ModbusMessage> msg);
    void on_mbResponse(QSharedPointer<const ModbusMessage> msgReq,
                       QSharedPointer<const ModbusMessage> msgResp);
    void on_filterChanged();
    void retranslateUi();

private:
    bool matchesFilter(QSharedPointer<const ModbusMessage> msg) const;
    void setupToolbar();

private:
    ModbusMultiServer&  _mbMultiServer;
    ModbusLogWidget*    _logWidget = nullptr;

    // Filter controls
    QLabel*      _labelUnitId   = nullptr;
    QSpinBox*    _unitIdFilter  = nullptr;   // 0 = all
    QLabel*      _labelFuncCode = nullptr;
    QComboBox*   _funcCodeFilter = nullptr;  // "All" or specific FC
    QLabel*      _labelRowLimit = nullptr;
    QComboBox*   _rowLimitCombo = nullptr;
    QToolButton* _pauseButton   = nullptr;
    QToolButton* _clearButton   = nullptr;
};

#endif // TRAFFICLOGWINDOW_H

