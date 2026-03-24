#include <QDateTime>
#include <QApplication>
#include <QItemSelectionModel>
#include <QPalette>
#include "fontutils.h"
#include "modbuslogwidget.h"
#include "outputtrafficwidget.h"
#include "ui_outputtrafficwidget.h"

///
/// \brief OutputTrafficWidget::OutputTrafficWidget
/// \param parent
///
OutputTrafficWidget::OutputTrafficWidget(QWidget* parent)
    : QFrame(parent)
    , ui(new Ui::OutputTrafficWidget)
{
    ui->setupUi(this);

    setFont(defaultMonospaceFont());
    setAutoFillBackground(true);
    setForegroundColor(Qt::black);
    setBackgroundColor(Qt::white);
    hideModbusMessage();

    connect(ui->logView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this, [&](const QItemSelection& sel) {
                if (!sel.indexes().isEmpty())
                    showModbusMessage(sel.indexes().first());
            });
}

///
/// \brief OutputTrafficWidget::~OutputTrafficWidget
///
OutputTrafficWidget::~OutputTrafficWidget()
{
    delete ui;
}

///
/// \brief OutputTrafficWidget::setup
/// \param dd
///
void OutputTrafficWidget::setup(const TrafficViewDefinitions& dd)
{
    _displayDefinition = dd;
    setLogViewLimit(dd.LogViewLimit);
}

///
/// \brief OutputTrafficWidget::backgroundColor
/// \return
///
QColor OutputTrafficWidget::backgroundColor() const
{
    return ui->logView->palette().color(QPalette::Base);
}

///
/// \brief OutputTrafficWidget::setBackgroundColor
/// \param clr
///
void OutputTrafficWidget::setBackgroundColor(const QColor& clr)
{
    auto pal = palette();
    pal.setColor(QPalette::Base, clr);
    pal.setColor(QPalette::Window, clr);
    setPalette(pal);
}

///
/// \brief OutputTrafficWidget::foregroundColor
/// \return
///
QColor OutputTrafficWidget::foregroundColor() const
{
    return ui->logView->palette().color(QPalette::Text);
}

///
/// \brief OutputTrafficWidget::setForegroundColor
/// \param clr
///
void OutputTrafficWidget::setForegroundColor(const QColor& clr)
{
    auto pal = ui->logView->palette();
    pal.setColor(QPalette::Text, clr);
    ui->logView->setPalette(pal);
}

///
/// \brief OutputTrafficWidget::font
/// \return
///
QFont OutputTrafficWidget::font() const
{
    return ui->logView->font();
}

///
/// \brief OutputTrafficWidget::setFont
/// \param font
///
void OutputTrafficWidget::setFont(const QFont& font)
{
    ui->logView->setFont(font);
    ui->modbusMsg->setFont(font);
}

///
/// \brief OutputTrafficWidget::logViewLimit
/// \return
///
int OutputTrafficWidget::logViewLimit() const
{
    return ui->logView->rowLimit();
}

///
/// \brief OutputTrafficWidget::setLogViewLimit
/// \param l
///
void OutputTrafficWidget::setLogViewLimit(int l)
{
    ui->logView->setRowLimit(l);
}

///
/// \brief OutputTrafficWidget::isLogEmpty
/// \return
///
bool OutputTrafficWidget::isLogEmpty() const
{
    return ui->logView->rowCount() == 0;
}

///
/// \brief OutputTrafficWidget::autoscrollLogView
/// \return
///
bool OutputTrafficWidget::autoscrollLogView() const
{
    return ui->logView->autoscroll();
}

///
/// \brief OutputTrafficWidget::setAutosctollLogView
/// \param on
///
void OutputTrafficWidget::setAutosctollLogView(bool on)
{
    ui->logView->setAutoscroll(on);
}

///
/// \brief OutputTrafficWidget::exportLogToTextFile
/// \param filePath
/// \return
///
bool OutputTrafficWidget::exportLogToTextFile(const QString& filePath)
{
    return ui->logView->exportToTextFile(filePath);
}

///
/// \brief OutputTrafficWidget::clearLogView
///
void OutputTrafficWidget::clearLogView()
{
    ui->logView->clear();
    ui->modbusMsg->clear();
    hideModbusMessage();
}

///
/// \brief OutputTrafficWidget::setLogViewState
/// \param state
///
void OutputTrafficWidget::setLogViewState(LogViewState state)
{
    ui->logView->setState(state);
}

///
/// \brief OutputTrafficWidget::changeEvent
/// \param event
///
void OutputTrafficWidget::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
}

///
/// \brief OutputTrafficWidget::updateTraffic
/// \param msg
///
void OutputTrafficWidget::updateTraffic(QSharedPointer<const ModbusMessage> msg)
{
    updateLogView(msg);
}

///
/// \brief OutputTrafficWidget::showModbusMessage
///
void OutputTrafficWidget::showModbusMessage(const QModelIndex& index)
{
    const auto msg = ui->logView->itemAt(index);
    if (msg) {
        if (ui->splitter->widget(1)->isHidden()) {
            ui->splitter->setSizes({1, 1});
            ui->splitter->widget(1)->show();
        }
        ui->modbusMsg->setModbusMessage(msg);
    }
}

///
/// \brief OutputTrafficWidget::hideModbusMessage
///
void OutputTrafficWidget::hideModbusMessage()
{
    ui->splitter->setSizes({1, 0});
    ui->splitter->widget(1)->hide();
}

///
/// \brief OutputTrafficWidget::updateLogView
///
void OutputTrafficWidget::updateLogView(QSharedPointer<const ModbusMessage> msg)
{
    ui->logView->addItem(msg);
}
