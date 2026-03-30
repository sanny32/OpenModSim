#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include "trafficlogwindow.h"
#include "modbusfunction.h"

///
/// \brief TrafficLogWindow::TrafficLogWindow
///
TrafficLogWindow::TrafficLogWindow(ModbusMultiServer& server, QWidget* parent)
    : QWidget(parent)
    , _mbMultiServer(server)
{
    setWindowTitle(tr("Traffic"));
    setupToolbar();

    connect(&_mbMultiServer, &ModbusMultiServer::request,  this, &TrafficLogWindow::on_mbRequest);
    connect(&_mbMultiServer, &ModbusMultiServer::response, this, &TrafficLogWindow::on_mbResponse);
}

///
/// \brief TrafficLogWindow::dataType
///
DataType TrafficLogWindow::dataType() const
{
    return _logWidget->dataType();
}

///
/// \brief TrafficLogWindow::setDataType
///
void TrafficLogWindow::setDataType(DataType type)
{
    _logWidget->setDataType(type);
}

///
/// \brief TrafficLogWindow::changeEvent
///
void TrafficLogWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslateUi();
    QWidget::changeEvent(event);
}

///
/// \brief TrafficLogWindow::on_mbRequest
///
void TrafficLogWindow::on_mbRequest(QSharedPointer<const ModbusMessage> msg)
{
    if (!msg || !matchesFilter(msg)) return;
    if (_logWidget->state() == LogViewState::Paused) return;
    _logWidget->addItem(msg);
}

///
/// \brief TrafficLogWindow::on_mbResponse
///
void TrafficLogWindow::on_mbResponse(QSharedPointer<const ModbusMessage> msgReq,
                                     QSharedPointer<const ModbusMessage> msgResp)
{
    if (_logWidget->state() == LogViewState::Paused) return;
    if (msgReq && matchesFilter(msgReq))
        _logWidget->addItem(msgReq);
    if (msgResp && matchesFilter(msgResp))
        _logWidget->addItem(msgResp);
}

///
/// \brief TrafficLogWindow::on_filterChanged
///
void TrafficLogWindow::on_filterChanged()
{
    // Filtering is applied live on incoming messages only (no re-filtering of history).
}

///
/// \brief TrafficLogWindow::matchesFilter
///
bool TrafficLogWindow::matchesFilter(QSharedPointer<const ModbusMessage> msg) const
{
    if (!msg) return false;

    // Unit ID filter (0 = all)
    const int unitId = _unitIdFilter->value();
    if (unitId != 0 && msg->deviceId() != unitId)
        return false;

    // Function code filter (index 0 = all)
    if (_funcCodeFilter->currentIndex() > 0) {
        const int fc = _funcCodeFilter->currentData().toInt();
        if (static_cast<int>(msg->functionCode()) != fc)
            return false;
    }

    return true;
}

///
/// \brief TrafficLogWindow::setupToolbar
///
void TrafficLogWindow::setupToolbar()
{
    // ── Toolbar ──────────────────────────────────────────────────────────────
    auto toolbar = new QWidget(this);
    auto tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(4, 2, 4, 2);
    tbLayout->setSpacing(6);

    // Unit ID filter
    _labelUnitId = new QLabel(tr("Unit ID:"), toolbar);
    _unitIdFilter = new QSpinBox(toolbar);
    _unitIdFilter->setRange(0, 247);
    _unitIdFilter->setValue(0);
    _unitIdFilter->setSpecialValueText(tr("All"));
    _unitIdFilter->setToolTip(tr("Filter by Unit Identifier (0 = all)"));
    _unitIdFilter->setFixedWidth(70);

    // Function code filter
    _labelFuncCode = new QLabel(tr("Function:"), toolbar);
    _funcCodeFilter = new QComboBox(toolbar);
    _funcCodeFilter->addItem(tr("All"), 0);
    _funcCodeFilter->addItem("FC01 Read Coils",               1);
    _funcCodeFilter->addItem("FC02 Read Discrete Inputs",     2);
    _funcCodeFilter->addItem("FC03 Read Holding Registers",   3);
    _funcCodeFilter->addItem("FC04 Read Input Registers",     4);
    _funcCodeFilter->addItem("FC05 Write Single Coil",        5);
    _funcCodeFilter->addItem("FC06 Write Single Register",    6);
    _funcCodeFilter->addItem("FC15 Write Multiple Coils",     15);
    _funcCodeFilter->addItem("FC16 Write Multiple Registers", 16);
    _funcCodeFilter->setFixedWidth(220);

    // Row limit
    _labelRowLimit = new QLabel(tr("Limit:"), toolbar);
    _rowLimitCombo = new QComboBox(toolbar);
    const QStringList limits = {"100", "500", "1000", "5000"};
    for (const auto& s : limits)
        _rowLimitCombo->addItem(s, s.toInt());
    _rowLimitCombo->setCurrentIndex(2);
    _rowLimitCombo->setFixedWidth(65);

    // Pause / Clear buttons
    _pauseButton = new QToolButton(toolbar);
    _pauseButton->setCheckable(true);
    _pauseButton->setIcon(QIcon(":/res/actionStopScript.png"));
    _pauseButton->setToolTip(tr("Pause"));
    _pauseButton->setAutoRaise(true);

    _clearButton = new QToolButton(toolbar);
    _clearButton->setIcon(QIcon(":/res/edit-delete.svg"));
    _clearButton->setToolTip(tr("Clear"));
    _clearButton->setAutoRaise(true);

    tbLayout->addWidget(_labelUnitId);
    tbLayout->addWidget(_unitIdFilter);
    tbLayout->addSpacing(8);
    tbLayout->addWidget(_labelFuncCode);
    tbLayout->addWidget(_funcCodeFilter);
    tbLayout->addSpacing(8);
    tbLayout->addWidget(_labelRowLimit);
    tbLayout->addWidget(_rowLimitCombo);
    tbLayout->addStretch();
    tbLayout->addWidget(_pauseButton);
    tbLayout->addWidget(_clearButton);

    // ── Log widget ───────────────────────────────────────────────────────────
    _logWidget = new ModbusLogWidget(this);
    _logWidget->setRowLimit(_rowLimitCombo->currentData().toInt());

    // ── Main layout ──────────────────────────────────────────────────────────
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(toolbar);
    mainLayout->addWidget(_logWidget);

    // ── Connections ──────────────────────────────────────────────────────────
    connect(_unitIdFilter,   QOverload<int>::of(&QSpinBox::valueChanged),
            this, &TrafficLogWindow::on_filterChanged);
    connect(_funcCodeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TrafficLogWindow::on_filterChanged);
    connect(_rowLimitCombo,  QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() {
                _logWidget->setRowLimit(_rowLimitCombo->currentData().toInt());
            });
    connect(_pauseButton, &QToolButton::toggled, this, [this](bool paused) {
        _logWidget->setState(paused ? LogViewState::Paused : LogViewState::Running);
        _pauseButton->setToolTip(paused ? tr("Resume") : tr("Pause"));
    });
    connect(_clearButton, &QToolButton::clicked, _logWidget, &ModbusLogWidget::clear);
}

///
/// \brief TrafficLogWindow::retranslateUi
///
void TrafficLogWindow::retranslateUi()
{
    setWindowTitle(tr("Traffic"));
    _labelUnitId->setText(tr("Unit ID:"));
    _unitIdFilter->setSpecialValueText(tr("All"));
    _unitIdFilter->setToolTip(tr("Filter by Unit Identifier (0 = all)"));
    _labelFuncCode->setText(tr("Function:"));
    _labelRowLimit->setText(tr("Limit:"));
    _pauseButton->setToolTip(_pauseButton->isChecked() ? tr("Resume") : tr("Pause"));
    _clearButton->setToolTip(tr("Clear"));

    // Retranslate FC combo first item
    _funcCodeFilter->setItemText(0, tr("All"));
}
