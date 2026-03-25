#ifndef OUTPUTTRAFFICWIDGET_H
#define OUTPUTTRAFFICWIDGET_H

#include <QSharedPointer>
#include <QVector>
#include <QFrame>
#include <QPrinter>
#include "displaydefinition.h"
#include "modbusmessage.h"

namespace Ui {
class OutputTrafficWidget;
}

class QModelIndex;

class OutputTrafficWidget : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(DataDisplayMode dataDisplayMode READ dataDisplayMode WRITE setDataDisplayMode)

public:
    explicit OutputTrafficWidget(QWidget* parent = nullptr);
    ~OutputTrafficWidget() override;

    DataDisplayMode dataDisplayMode() const;
    void setDataDisplayMode(DataDisplayMode mode);

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& clr);

    QColor foregroundColor() const;
    void setForegroundColor(const QColor& clr);

    QFont font() const;
    void setFont(const QFont& font);

    int logViewLimit() const;
    void setLogViewLimit(int l);
    bool isLogEmpty() const;

    bool autoscrollLogView() const;
    void setAutosctollLogView(bool on);
    bool exportLogToTextFile(const QString& filePath);

    void print(QPrinter* printer);

    void updateTraffic(QSharedPointer<const ModbusMessage> msg);
    void updateTrafficBatch(const QVector<QSharedPointer<const ModbusMessage>>& messages);

public slots:
    void clearLogView();
    void setLogViewState(LogViewState state);

protected:
    void changeEvent(QEvent* event) override;

private:
    void showModbusMessage(const QModelIndex& index);
    void hideModbusMessage();
    void updateLogView(QSharedPointer<const ModbusMessage> msg);
    void updateLogViewBatch(const QVector<QSharedPointer<const ModbusMessage>>& messages);

private:
    Ui::OutputTrafficWidget* ui;
};

#endif // OUTPUTTRAFFICWIDGET_H
