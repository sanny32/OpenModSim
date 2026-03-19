#ifndef OUTPUTTRAFFICWIDGET_H
#define OUTPUTTRAFFICWIDGET_H

#include <QSharedPointer>
#include <QWidget>
#include "displaydefinition.h"
#include "modbusmessage.h"

namespace Ui {
class OutputTrafficWidget;
}

class QModelIndex;

class OutputTrafficWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OutputTrafficWidget(QWidget* parent = nullptr);
    ~OutputTrafficWidget() override;

    void setup(const TrafficViewDefinitions& dd);

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& clr);

    QColor foregroundColor() const;
    void setForegroundColor(const QColor& clr);

    QFont font() const;
    void setFont(const QFont& font);

    int logViewLimit() const;
    void setLogViewLimit(int l);

    bool autoscrollLogView() const;
    void setAutosctollLogView(bool on);
    bool exportLogToTextFile(const QString& filePath);

    void updateTraffic(QSharedPointer<const ModbusMessage> msg);

public slots:
    void clearLogView();
    void setLogViewState(LogViewState state);

protected:
    void changeEvent(QEvent* event) override;

private:
    void showModbusMessage(const QModelIndex& index);
    void hideModbusMessage();
    void updateLogView(QSharedPointer<const ModbusMessage> msg);

private:
    Ui::OutputTrafficWidget* ui;
    TrafficViewDefinitions _displayDefinition;
};

#endif // OUTPUTTRAFFICWIDGET_H
