#include <QEvent>
#include "statisticwidget.h"
#include "ui_statisticwidget.h"

///
/// \brief StatisticWidget::StatisticWidget
/// \param parent
///
StatisticWidget::StatisticWidget(QWidget *parent) :
      QWidget(parent)
    , ui(new Ui::StatisticWidget)
    ,_requests(0)
    ,_responses(0)
{
    ui->setupUi(this);
}

///
/// \brief StatisticWidget::~StatisticWidget
///
StatisticWidget::~StatisticWidget()
{
    delete ui;
}

///
/// \brief StatisticWidget::changeEvent
/// \param event
///
void StatisticWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        setLogState(_logState);
        updateStatistic();
    }

    QWidget::changeEvent(event);
}

///
/// \brief StatisticWidget::increaseNumberOfPolls
///
void StatisticWidget::increaseRequests()
{
    _requests++;
    updateStatistic();

    emit numberRequestsChanged(_requests);
}

///
/// \brief StatisticWidget::increaseValidSlaveResponses
///
void StatisticWidget::increaseResponses()
{
   _responses++;
   updateStatistic();

   emit numberResposesChanged(_responses);
}

///
/// \brief StatisticWidget::resetCtrls
///
void StatisticWidget::resetCtrs()
{
    _requests = 0;
    _responses = 0;

    updateStatistic();

    emit numberRequestsChanged(_requests);
    emit numberResposesChanged(_responses);
}

///
/// \brief StatisticWidget::logState
/// \return
///
LogViewState StatisticWidget::logState() const
{
    return _logState;
}

///
/// \brief StatisticWidget::setLogViewState
/// \param state
///
void StatisticWidget::setLogState(LogViewState state)
{
    _logState = state;
    switch (state)
    {
        case LogViewState::Unknown:
            ui->pushButtonPause->setEnabled(false);
            ui->pushButtonPause->setText(tr("Pause Logging"));
        break;
        case LogViewState::Running:
            ui->pushButtonPause->setEnabled(true);
            ui->pushButtonPause->setText(tr("Pause Logging"));
        break;
        case LogViewState::Paused:
            ui->pushButtonPause->setEnabled(true);
            ui->pushButtonPause->setText(tr("Resume Logging"));
        break;
    }
}

///
/// \brief StatisticWidget::on_pushButtonResetCtrs_clicked
///
void StatisticWidget::on_pushButtonResetCtrs_clicked()
{
    resetCtrs();
    emit ctrsReseted();
}

///
/// \brief StatisticWidget::on_pushButtonPause_clicked
///
void StatisticWidget::on_pushButtonPause_clicked()
{
    switch (_logState) {
    case LogViewState::Unknown: break;
    case LogViewState::Running: setLogState(LogViewState::Paused); break;
    case LogViewState::Paused: setLogState(LogViewState::Running); break;
    }

    emit logStateChanged(_logState);
}

///
/// \brief StatisticWidget::updateStatistic
///
void StatisticWidget::updateStatistic()
{
    ui->labelNumberRequests->setText(QString(tr("Requests: %1")).arg(_requests));
    ui->labelNumberResponses->setText(QString(tr("Responses: %1")).arg(_responses));
}
