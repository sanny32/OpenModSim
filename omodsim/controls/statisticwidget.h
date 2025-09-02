#ifndef STATISTICWIDGET_H
#define STATISTICWIDGET_H

#include <QWidget>
#include "enums.h"

namespace Ui {
class StatisticWidget;
}

///
/// \brief The StatisticWidget class
///
class StatisticWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StatisticWidget(QWidget *parent = nullptr);
    ~StatisticWidget();

    uint numberRequets() const { return _requests; }
    uint numberResposes() const { return _responses; }

    void increaseRequests();
    void increaseResponses();
    void resetCtrs();

    LogViewState logState() const;
    void setLogState(LogViewState state);

signals:
    void numberRequestsChanged(uint value);
    void numberResposesChanged(uint value);
    void ctrsReseted();
    void logStateChanged(LogViewState state);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_pushButtonResetCtrs_clicked();
    void on_pushButtonPause_clicked();

private:
    void updateStatistic();

private:
    Ui::StatisticWidget *ui;

private:
    uint _requests;
    uint _responses;
    LogViewState _logState;
};

#endif // STATISTICWIDGET_H
