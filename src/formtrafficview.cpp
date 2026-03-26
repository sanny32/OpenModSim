#include <QPainter>
#include <QPrinter>
#include <QPalette>
#include <QDateTime>
#include <QHelpEngine>
#include <QHelpContentWidget>
#include <QSpinBox>
#include <QLabel>
#include <QSizePolicy>
#include <QCheckBox>
#include <QSignalBlocker>
#include <QFileDialog>
#include <QMessageBox>
#include <QVector>
#include "mainwindow.h"
#include "modbusmessages.h"
#include "serialportutils.h"
#include "formtrafficview.h"
#include "ui_formtrafficview.h"

namespace {
constexpr int TrafficUiFlushIntervalMs = 20;
constexpr int TrafficUiFlushChunkSize = 300;
}

///
/// \brief FormTrafficView::FormTrafficView
/// \param num
/// \param parent
///
FormTrafficView::FormTrafficView(ModbusMultiServer& server, MainWindow* parent)
    : QWidget(parent)
    , ui(new Ui::FormTrafficView)
    , _mbMultiServer(server)
{
    Q_ASSERT(parent != nullptr);

    ui->setupUi(this);
    setWindowIcon(QIcon(":/res/actionShowTraffic.png"));
    setupToolbarActions();
    setupFilterControls();
    setupToolbarLayout();
    initializeDisplayDefinition();

    ui->outputWidget->setFocus();

    setLogViewState(server.isConnected() ? LogViewState::Running : LogViewState::Unknown);

    setupServerConnections();

    _logUiFlushTimer = new QTimer(this);
    _logUiFlushTimer->setSingleShot(true);
    _logUiFlushTimer->setInterval(TrafficUiFlushIntervalMs);
    connect(_logUiFlushTimer, &QTimer::timeout, this, &FormTrafficView::on_logUiFlushTimeout);

    ui->outputWidget->setLogViewLimit(_displayDefinition.LogViewLimit);
    updateSourceFilter();
    updateExportActionState();
}

///
/// \brief FormTrafficView::~FormTrafficView
///
FormTrafficView::~FormTrafficView()
{
    delete ui;
}

///
/// \brief FormTrafficView::isLogEmpty
/// \return
///
bool FormTrafficView::isLogEmpty() const
{
    return ui->outputWidget->isLogEmpty();
}

///
/// \brief FormTrafficView::print
/// \param printer
///
void FormTrafficView::print(QPrinter* printer)
{
    if (!printer) return;

    const QString unitText = (_unitIdFilter && _unitIdFilter->value() >= 0)
        ? QString::number(_unitIdFilter->value())
        : tr("All");

    const QString funcText = _funcCodeFilter
        ? _funcCodeFilter->currentText()
        : tr("All");

    const QString sourceText = _sourceFilter
        ? _sourceFilter->currentText()
        : tr("All");

    const bool exceptionsOnly = _exceptionsFilter && _exceptionsFilter->isChecked();

    QString filterText = QString(tr("Unit: %1\nFunction: %2\nSource: %3"))
        .arg(unitText, funcText, sourceText);
    if (exceptionsOnly)
        filterText += QStringLiteral("\n") + tr("Exceptions Only");

    const auto pageRect   = printer->pageRect(QPrinter::DevicePixel).toRect();
    const int  marginX    = qMax(pageRect.width()  / 40, 10);
    const int  marginY    = qMax(pageRect.height() / 40, 10);
    const int  pageWidth  = pageRect.width()  - 2 * marginX;
    const int  pageHeight = pageRect.height() - 2 * marginY;
    const int  cx         = pageRect.left() + marginX;
    const int  cy         = pageRect.top()  + marginY;

    QPainter painter(printer);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    const auto textTime   = QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat);
    auto rcTime   = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextSingleLine, textTime);
    auto rcFilter = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextWordWrap,  filterText);
    rcTime.moveTopRight({ pageRect.right() - marginX, cy });

    const int lineGap    = qMax(pageRect.height() / 60, 10);
    const int lineY      = rcFilter.bottom() + lineGap;
    const int pageNumH   = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextSingleLine, QStringLiteral("0")).height();
    auto      rcOutput   = pageRect.adjusted(marginX, 0, -marginX, -marginY);
    rcOutput.setTop(lineY + lineGap);
    rcOutput.setBottom(rcOutput.bottom() - pageNumH - lineGap);

    const int totalRows = ui->outputWidget->rowCount();
    int startRow = 0;
    int pageNum  = 1;
    do {
        painter.drawText(rcTime,   Qt::TextSingleLine, textTime);
        painter.drawText(rcFilter, Qt::TextWordWrap,   filterText);
        painter.drawLine(pageRect.left() + marginX, lineY, pageRect.right() - marginX, lineY);
        startRow = ui->outputWidget->paint(rcOutput, painter, startRow);

        const auto textPage = QString::number(pageNum++);
        auto rcPage = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextSingleLine, textPage);
        rcPage.moveBottomRight({ pageRect.right() - marginX, pageRect.bottom() - marginY });
        painter.drawText(rcPage, Qt::TextSingleLine, textPage);

        if (startRow < totalRows)
            printer->newPage();
    } while (startRow < totalRows);
}

///
/// \brief FormTrafficView::saveSettings
///
void FormTrafficView::saveSettings(QSettings& out) const
{
    out << const_cast<FormTrafficView*>(this);
}

///
/// \brief FormTrafficView::loadSettings
///
void FormTrafficView::loadSettings(QSettings& in)
{
    in >> this;
}

///
/// \brief FormTrafficView::saveXml
///
void FormTrafficView::saveXml(QXmlStreamWriter& xml) const
{
    xml << const_cast<FormTrafficView*>(this);
}

///
/// \brief FormTrafficView::loadXml
///
void FormTrafficView::loadXml(QXmlStreamReader& xml)
{
    xml >> this;
}

///
/// \brief FormTrafficView::updateExportActionState
///
void FormTrafficView::updateExportActionState()
{
    ui->actionExportTrafficLog->setEnabled(!ui->outputWidget->isLogEmpty() || !_pendingLogViewUpdates.isEmpty());
}

///
/// \brief FormTrafficView::on_logUiFlushTimeout
///
void FormTrafficView::on_logUiFlushTimeout()
{
    if (_pendingLogViewUpdates.isEmpty()) {
        updateExportActionState();
        return;
    }

    const int batchSize = qMin(TrafficUiFlushChunkSize, _pendingLogViewUpdates.size());
    QVector<QSharedPointer<const ModbusMessage>> batch;
    batch.reserve(batchSize);
    for (int i = 0; i < batchSize; ++i)
        batch.push_back(_pendingLogViewUpdates.dequeue());

    ui->outputWidget->updateTrafficBatch(batch);

    if (!_pendingLogViewUpdates.isEmpty())
        _logUiFlushTimer->start();

    updateExportActionState();
}

///
/// \brief FormTrafficView::changeEvent
/// \param e
///
void FormTrafficView::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    QWidget::changeEvent(e);
}

///
/// \brief FormTrafficView::closeEvent
/// \param event
///
void FormTrafficView::closeEvent(QCloseEvent* event)
{
    emit closing();
    QWidget::closeEvent(event);
}

///
/// \brief FormTrafficView::mouseDoubleClickEvent
/// \param event
/// \return
///
///
/// \brief FormTrafficView::displayDefinition
/// \return
///
TrafficViewDefinitions FormTrafficView::displayDefinition() const
{
    TrafficViewDefinitions dd = _displayDefinition;
    dd.FormName = windowTitle();

    if (_unitIdFilter) {
        dd.UnitFilter = static_cast<qint16>(_unitIdFilter->value());
    }

    if (_funcCodeFilter) {
        dd.FunctionCodeFilter = static_cast<qint16>(_funcCodeFilter->currentData().toInt());
    }

    dd.LogViewLimit = ui->outputWidget->logViewLimit();

    if (_exceptionsFilter) {
        dd.ExceptionsOnly = _exceptionsFilter->isChecked();
    }

    if (_autoscrollCheck) {
        dd.Autoscroll = _autoscrollCheck->isChecked();
    }

    dd.HexView = ui->actionHexView->isChecked();

    dd.normalize();
    return dd;
}

///
/// \brief FormTrafficView::setDisplayDefinition
/// \param dd
///
void FormTrafficView::setDisplayDefinition(const TrafficViewDefinitions& dd)
{
    TrafficViewDefinitions next = dd;
    if(!next.FormName.isEmpty())
        setWindowTitle(next.FormName);
    else
        next.FormName = windowTitle();

    next.normalize();
    _displayDefinition = next;

    if (_unitIdFilter) {
        QSignalBlocker b(_unitIdFilter);
        _unitIdFilter->setValue(_displayDefinition.UnitFilter);
    }

    if (_funcCodeFilter) {
        QSignalBlocker b(_funcCodeFilter);
        int idx = _funcCodeFilter->findData(_displayDefinition.FunctionCodeFilter);
        if (idx < 0)
            idx = 0;
        _funcCodeFilter->setCurrentIndex(idx);
    }

    if (_rowLimitCombo) {
        QSignalBlocker b(_rowLimitCombo);
        _rowLimitCombo->setCurrentValue(_displayDefinition.LogViewLimit);
    }

    if (_exceptionsFilter) {
        QSignalBlocker b(_exceptionsFilter);
        _exceptionsFilter->setChecked(_displayDefinition.ExceptionsOnly);
    }

    if (_autoscrollCheck) {
        QSignalBlocker b(_autoscrollCheck);
        _autoscrollCheck->setChecked(_displayDefinition.Autoscroll);
    }

    {
        QSignalBlocker b(ui->actionHexView);
        ui->actionHexView->setChecked(_displayDefinition.HexView);
    }
    ui->outputWidget->setDataDisplayMode(_displayDefinition.HexView ? DataDisplayMode::Hex : DataDisplayMode::UInt16);

    ui->outputWidget->setAutosctollLogView(_displayDefinition.Autoscroll);

    trimTrafficBufferToLimit();
    ui->outputWidget->setLogViewLimit(_displayDefinition.LogViewLimit);
    rebuildVisibleTraffic();
}

///
/// \brief FormTrafficView::backgroundColor
/// \return
///
QColor FormTrafficView::backgroundColor() const
{
    return ui->outputWidget->backgroundColor();
}

///
/// \brief FormTrafficView::setBackgroundColor
/// \param clr
///
void FormTrafficView::setBackgroundColor(const QColor& clr)
{
    ui->outputWidget->setBackgroundColor(clr);
}

///
/// \brief FormTrafficView::foregroundColor
/// \return
///
QColor FormTrafficView::foregroundColor() const
{
    return ui->outputWidget->foregroundColor();
}

///
/// \brief FormTrafficView::setForegroundColor
/// \param clr
///
void FormTrafficView::setForegroundColor(const QColor& clr)
{
    ui->outputWidget->setForegroundColor(clr);
}

///
/// \brief FormTrafficView::font
/// \return
///
QFont FormTrafficView::font() const
{
   return ui->outputWidget->font();
}

///
/// \brief FormTrafficView::setFont
/// \param font
///
void FormTrafficView::setFont(const QFont& font)
{
    ui->outputWidget->setFont(font);
}

///
/// \brief FormTrafficView::logViewState
/// \return
///
LogViewState FormTrafficView::logViewState() const
{
    return _logViewState;
}

///
/// \brief FormTrafficView::setLogViewState
/// \param state
///
void FormTrafficView::setLogViewState(LogViewState state)
{
    if(_logViewState == state)
        return;

    _logViewState = state;
    ui->outputWidget->setLogViewState(state);
    ui->actionPauseTraffic->blockSignals(true);
    ui->actionPauseTraffic->setChecked(state == LogViewState::Paused);
    ui->actionPauseTraffic->blockSignals(false);
    ui->actionPauseTraffic->setEnabled(state != LogViewState::Unknown);
    emit logViewStateChanged(state);
}

///
/// \brief FormTrafficView::setDisplayDefinitionSilent
/// Updates filter controls and definition without emitting definitionChanged.
/// Used for peer sync to prevent signal loops.
///
void FormTrafficView::setDisplayDefinitionSilent(const TrafficViewDefinitions& dd)
{
    _displayDefinition.UnitFilter = dd.UnitFilter;
    _displayDefinition.FunctionCodeFilter = dd.FunctionCodeFilter;
    _displayDefinition.LogViewLimit = dd.LogViewLimit;
    _displayDefinition.ExceptionsOnly = dd.ExceptionsOnly;
    _displayDefinition.Autoscroll = dd.Autoscroll;
    _displayDefinition.HexView = dd.HexView;

    if(_unitIdFilter) {
        QSignalBlocker b(_unitIdFilter);
        _unitIdFilter->setValue(dd.UnitFilter);
    }
    if(_funcCodeFilter) {
        QSignalBlocker b(_funcCodeFilter);
        const int idx = _funcCodeFilter->findData(dd.FunctionCodeFilter);
        _funcCodeFilter->setCurrentIndex(idx < 0 ? 0 : idx);
    }
    if(_rowLimitCombo) {
        QSignalBlocker b(_rowLimitCombo);
        _rowLimitCombo->setCurrentValue(static_cast<int>(dd.LogViewLimit));
    }
    if(_exceptionsFilter) {
        QSignalBlocker b(_exceptionsFilter);
        _exceptionsFilter->setChecked(dd.ExceptionsOnly);
    }
    if(_autoscrollCheck) {
        QSignalBlocker b(_autoscrollCheck);
        _autoscrollCheck->setChecked(dd.Autoscroll);
    }
    {
        QSignalBlocker b(ui->actionHexView);
        ui->actionHexView->setChecked(dd.HexView);
    }
    ui->outputWidget->setDataDisplayMode(dd.HexView ? DataDisplayMode::Hex : DataDisplayMode::UInt16);
    ui->outputWidget->setAutosctollLogView(dd.Autoscroll);

    trimTrafficBufferToLimit();
    ui->outputWidget->setLogViewLimit(dd.LogViewLimit);
    rebuildVisibleTraffic();
}

///
/// \brief FormTrafficView::linkTo
/// Bidirectionally syncs filter settings and pause state with \a other.
///
void FormTrafficView::linkTo(FormTrafficView* other)
{
    if(!other) return;
    connect(this,  &FormTrafficView::definitionChanged, other, [this, other]() {
        other->setDisplayDefinitionSilent(displayDefinition());
    });
    connect(other, &FormTrafficView::definitionChanged, this, [this, other]() {
        setDisplayDefinitionSilent(other->displayDefinition());
    });
    connect(this,  &FormTrafficView::logViewStateChanged, other, &FormTrafficView::setLogViewState);
    connect(other, &FormTrafficView::logViewStateChanged, this,  &FormTrafficView::setLogViewState);
}

///
/// \brief FormTrafficView::show
///
void FormTrafficView::show()
{
    QWidget::show();
    connectEditSlots();

    emit showed();
}

///
/// \brief FormTrafficView::connectEditSlots
///
void FormTrafficView::connectEditSlots()
{
}

///
/// \brief FormTrafficView::disconnectEditSlots
///
void FormTrafficView::disconnectEditSlots()
{
}

///
/// \brief FormTrafficView::on_mbConnected
///
void FormTrafficView::on_mbConnected(const ConnectionDetails&)
{
    updateSourceFilter();
    clearTrafficLog();

    if(logViewState() == LogViewState::Unknown) {
        setLogViewState(LogViewState::Running);
    }
}

///
/// \brief FormTrafficView::on_mbDisconnected
///
void FormTrafficView::on_mbDisconnected(const ConnectionDetails&)
{
    updateSourceFilter();
    if(!_mbMultiServer.isConnected()) {
        setLogViewState(LogViewState::Unknown);
    }
}

///
/// \brief FormTrafficView::matchesTrafficFilter
/// \param msg
/// \return
///
QString FormTrafficView::sourceFilterText(const ConnectionDetails& cd) const
{
    if (cd.Type == ConnectionType::Tcp)
        return tr("Modbus/TCP Srv %1:%2").arg(cd.TcpParams.IPAddress, QString::number(cd.TcpParams.ServicePort));

    return tr("Port %1:%2:%3:%4:%5").arg(cd.SerialParams.PortName,
                                          QString::number(cd.SerialParams.BaudRate),
                                          QString::number(cd.SerialParams.WordLength),
                                          Parity_toString(cd.SerialParams.Parity),
                                          QString::number(cd.SerialParams.StopBits));
}

///
/// \brief FormTrafficView::updateSourceFilter
///
void FormTrafficView::updateSourceFilter()
{
    if (!_sourceFilter)
        return;

    const QVariant selectedData = _sourceFilter->currentData();
    {
        QSignalBlocker b(_sourceFilter);
        _sourceFilter->clear();
        _sourceFilter->addItem(tr("All"), QVariant());
        const auto activeConnections = _mbMultiServer.connections();
        for (const auto& cd : activeConnections)
            _sourceFilter->addItem(sourceFilterText(cd), QVariant::fromValue(cd));
    }

    int idx = _sourceFilter->findData(selectedData);
    if (idx < 0)
        idx = 0;
    _sourceFilter->setCurrentIndex(idx);
}

///
/// \brief FormTrafficView::matchesTrafficFilter
///
bool FormTrafficView::matchesTrafficFilter(const ConnectionDetails& cd,
                                            QSharedPointer<const ModbusMessage> filterMsg,
                                            QSharedPointer<const ModbusMessage> displayMsg) const
{
    if (!filterMsg)
        return false;

    if (!_unitIdFilter || !_funcCodeFilter)
        return true;

    const int unitId = _unitIdFilter->value();
    if (unitId != -1 && filterMsg->deviceId() != unitId)
        return false;

    if (_funcCodeFilter->currentIndex() > 0) {
        const int fc = _funcCodeFilter->currentData().toInt();
        if (static_cast<int>(filterMsg->functionCode()) != fc)
            return false;
    }

    if (_sourceFilter && _sourceFilter->currentIndex() > 0) {
        const auto selected = _sourceFilter->currentData().value<ConnectionDetails>();
        if (!(selected == cd))
            return false;
    }

    if (_exceptionsFilter && _exceptionsFilter->isChecked()) {
        const auto& msg = displayMsg ? displayMsg : filterMsg;
        if (!msg->isException())
            return false;
    }

    return true;
}

///
/// \brief FormTrafficView::on_mbRequest
/// \param msg
///
void FormTrafficView::on_mbRequest(const ConnectionDetails& cd, QSharedPointer<const ModbusMessage> msg)
{
    appendTrafficEntry(cd, msg, msg, true);
}

///
/// \brief FormTrafficView::on_mbResponse
/// \param msgReq
/// \param msgResp
///
void FormTrafficView::on_mbResponse(const ConnectionDetails& cd, QSharedPointer<const ModbusMessage> msgReq, QSharedPointer<const ModbusMessage> msgResp)
{
    appendTrafficEntry(cd, msgResp, msgReq ? msgReq : msgResp, false);
}

///
/// \brief FormTrafficView::on_mbDataChanged
///
void FormTrafficView::on_mbDataChanged(quint8 deviceId, const QModbusDataUnit&)
{
    Q_UNUSED(deviceId);
}

///
/// \brief FormTrafficView::on_mbDefinitionsChanged
/// \param defs
///
void FormTrafficView::on_mbDefinitionsChanged(const ModbusDefinitions& defs)
{
    Q_UNUSED(defs);
}

///
/// \brief FormTrafficView::on_actionPauseTraffic_toggled
/// \param checked
///
void FormTrafficView::on_actionPauseTraffic_toggled(bool checked)
{
    if (_logViewState == LogViewState::Unknown)
        return;
    setLogViewState(checked ? LogViewState::Paused : LogViewState::Running);
}

///
/// \brief FormTrafficView::on_actionClearTraffic_triggered
///
void FormTrafficView::on_actionClearTraffic_triggered()
{
    clearTrafficLog();
}

///
/// \brief FormTrafficView::on_actionExportTrafficLog_triggered
///
void FormTrafficView::on_actionExportTrafficLog_triggered()
{
    const auto filename = QFileDialog::getSaveFileName(this, QString(), QString(), tr("Text files (*.txt)"));
    if (filename.isEmpty())
        return;

    flushPendingTrafficUiAll();

    if (ui->outputWidget->exportLogToTextFile(filename))
        QMessageBox::information(this, windowTitle(), tr("Log exported successfully to file %1").arg(filename));
    else
        QMessageBox::critical(this, windowTitle(), tr("Export log error!"));
}

///
/// \brief FormTrafficView::on_actionHexView_toggled
/// \param checked
///
void FormTrafficView::on_actionHexView_toggled(bool checked)
{
    ui->outputWidget->setDataDisplayMode(checked ? DataDisplayMode::Hex : DataDisplayMode::UInt16);
}

///
/// \brief FormTrafficView::setupToolbarActions
///
void FormTrafficView::setupToolbarActions()
{
    ui->toolBarTraffic->setVisible(true);

    ui->toolBarTraffic->removeAction(ui->actionPauseTraffic);
    ui->toolBarTraffic->removeAction(ui->actionClearTraffic);
    ui->toolBarTraffic->removeAction(ui->actionExportTrafficLog);
    ui->toolBarTraffic->removeAction(ui->actionHexView);
}

///
/// \brief FormTrafficView::setupFilterControls
///
void FormTrafficView::setupFilterControls()
{
    _labelUnitId = new QLabel(ui->toolBarTraffic);
    _labelUnitId->setText(tr("Unit:"));

    _unitIdFilter = new QSpinBox(ui->toolBarTraffic);
    _unitIdFilter->setRange(-1, 255);
    _unitIdFilter->setValue(-1);
    _unitIdFilter->setSpecialValueText(tr("All"));
    _unitIdFilter->setToolTip(tr("-1 = all unit ids"));
    connect(_unitIdFilter, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        if (_unitIdFilter)
            _displayDefinition.UnitFilter = static_cast<qint16>(_unitIdFilter->value());
        rebuildVisibleTraffic();
        emit definitionChanged();
    });

    _labelFuncCode = new QLabel(ui->toolBarTraffic);
    _labelFuncCode->setText(tr("Function:"));

    _funcCodeFilter = new QComboBox(ui->toolBarTraffic);
    _funcCodeFilter->addItem(tr("All"), -1);
    _funcCodeFilter->addItem(QStringLiteral("01 Read Coils"), static_cast<int>(QModbusPdu::ReadCoils));
    _funcCodeFilter->addItem(QStringLiteral("02 Read Discrete Inputs"), static_cast<int>(QModbusPdu::ReadDiscreteInputs));
    _funcCodeFilter->addItem(QStringLiteral("03 Read Holding Registers"), static_cast<int>(QModbusPdu::ReadHoldingRegisters));
    _funcCodeFilter->addItem(QStringLiteral("04 Read Input Registers"), static_cast<int>(QModbusPdu::ReadInputRegisters));
    _funcCodeFilter->addItem(QStringLiteral("05 Write Single Coil"), static_cast<int>(QModbusPdu::WriteSingleCoil));
    _funcCodeFilter->addItem(QStringLiteral("06 Write Single Register"), static_cast<int>(QModbusPdu::WriteSingleRegister));
    _funcCodeFilter->addItem(QStringLiteral("15 Write Multiple Coils"), static_cast<int>(QModbusPdu::WriteMultipleCoils));
    _funcCodeFilter->addItem(QStringLiteral("16 Write Multiple Registers"), static_cast<int>(QModbusPdu::WriteMultipleRegisters));
    _funcCodeFilter->addItem(QStringLiteral("22 Mask Write Register"), static_cast<int>(QModbusPdu::MaskWriteRegister));
    _funcCodeFilter->addItem(QStringLiteral("23 Read/Write Multiple Registers"), static_cast<int>(QModbusPdu::ReadWriteMultipleRegisters));
    connect(_funcCodeFilter, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        if (_funcCodeFilter)
            _displayDefinition.FunctionCodeFilter = static_cast<qint16>(_funcCodeFilter->currentData().toInt());
        rebuildVisibleTraffic();
        emit definitionChanged();
    });

    _labelSource = new QLabel(ui->toolBarTraffic);
    _labelSource->setText(tr("Source:"));

    _sourceFilter = new QComboBox(ui->toolBarTraffic);
    _sourceFilter->addItem(tr("All"), QVariant());
    _sourceFilter->setMinimumWidth(220);
    _sourceFilter->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(_sourceFilter, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        rebuildVisibleTraffic();
    });

    _exceptionsFilter = new QCheckBox(tr("Exceptions Only"), ui->toolBarTraffic);
    _exceptionsFilter->setToolTip(tr("Show only responses with Modbus exception"));
    connect(_exceptionsFilter, &QCheckBox::toggled, this, [this](bool checked) {
        _displayDefinition.ExceptionsOnly = checked;
        rebuildVisibleTraffic();
        emit definitionChanged();
    });

    _autoscrollCheck = new QCheckBox(tr("Autoscroll"), ui->toolBarTraffic);
    _autoscrollCheck->setToolTip(tr("Automatically scroll to the latest entry"));
    connect(_autoscrollCheck, &QCheckBox::toggled, this, [this](bool checked) {
        _displayDefinition.Autoscroll = checked;
        ui->outputWidget->setAutosctollLogView(checked);
        emit definitionChanged();
    });

    _labelRowLimit = new QLabel(ui->toolBarTraffic);
    _labelRowLimit->setText(tr("Rows:"));

    _rowLimitCombo = new NumericComboBox(ui->toolBarTraffic);
    _rowLimitCombo->setRange(30, 10000);
    _rowLimitCombo->addValue(30);
    _rowLimitCombo->addValue(100);
    _rowLimitCombo->addValue(300);
    _rowLimitCombo->addValue(1000);
    connect(_rowLimitCombo, &NumericComboBox::currentValueChanged, this, [this](int value) {
        _displayDefinition.LogViewLimit = static_cast<quint16>(value);
        trimTrafficBufferToLimit();
        ui->outputWidget->setLogViewLimit(value);
        rebuildVisibleTraffic();
        emit definitionChanged();
    });
}

///
/// \brief FormTrafficView::setupToolbarLayout
///
void FormTrafficView::setupToolbarLayout()
{
    ui->toolBarTraffic->addWidget(_labelUnitId);
    addToolbarSpacer(3);
    ui->toolBarTraffic->addWidget(_unitIdFilter);
    addToolbarSpacer(8);

    ui->toolBarTraffic->addWidget(_labelFuncCode);
    addToolbarSpacer(3);
    ui->toolBarTraffic->addWidget(_funcCodeFilter);
    addToolbarSpacer(8);

    ui->toolBarTraffic->addWidget(_labelSource);
    addToolbarSpacer(3);
    ui->toolBarTraffic->addWidget(_sourceFilter);
    addToolbarSpacer(8);

    ui->toolBarTraffic->addWidget(_exceptionsFilter);

    addToolbarSpacer(6);
    ui->toolBarTraffic->addSeparator();
    addToolbarSpacer(6);

    ui->toolBarTraffic->addWidget(_labelRowLimit);
    addToolbarSpacer(3);
    ui->toolBarTraffic->addWidget(_rowLimitCombo);
    addToolbarSpacer(8);

    ui->toolBarTraffic->addWidget(_autoscrollCheck);
    addToolbarSpacer(6);
    ui->toolBarTraffic->addSeparator();
    addToolbarSpacer(6);
    ui->toolBarTraffic->addAction(ui->actionHexView);

    _trafficFilterStretch = new QWidget(ui->toolBarTraffic);
    _trafficFilterStretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->toolBarTraffic->addWidget(_trafficFilterStretch);
    ui->toolBarTraffic->addAction(ui->actionPauseTraffic);
    ui->toolBarTraffic->addSeparator();
    ui->toolBarTraffic->addAction(ui->actionClearTraffic);
    ui->toolBarTraffic->addSeparator();
    ui->toolBarTraffic->addAction(ui->actionExportTrafficLog);
}

///
/// \brief FormTrafficView::initializeDisplayDefinition
///
void FormTrafficView::initializeDisplayDefinition()
{
    _displayDefinition.FormName = windowTitle();
    _displayDefinition.UnitFilter = -1;
    _displayDefinition.FunctionCodeFilter = -1;
    _displayDefinition.LogViewLimit = ui->outputWidget->logViewLimit();
    _displayDefinition.HexView = true;
    _displayDefinition.normalize();

    if (_unitIdFilter)
        _unitIdFilter->setValue(_displayDefinition.UnitFilter);

    if (_funcCodeFilter) {
        int idx = _funcCodeFilter->findData(_displayDefinition.FunctionCodeFilter);
        if (idx < 0)
            idx = 0;
        _funcCodeFilter->setCurrentIndex(idx);
    }

    if (_rowLimitCombo)
        _rowLimitCombo->setCurrentValue(_displayDefinition.LogViewLimit);

    if (_exceptionsFilter)
        _exceptionsFilter->setChecked(_displayDefinition.ExceptionsOnly);
}

///
/// \brief FormTrafficView::setupServerConnections
///
void FormTrafficView::setupServerConnections()
{
    connect(&_mbMultiServer, &ModbusMultiServer::requestOnConnection, this, &FormTrafficView::on_mbRequest);
    connect(&_mbMultiServer, &ModbusMultiServer::responseOnConnection, this, &FormTrafficView::on_mbResponse);
    connect(&_mbMultiServer, &ModbusMultiServer::connected, this, &FormTrafficView::on_mbConnected);
    connect(&_mbMultiServer, &ModbusMultiServer::disconnected, this, &FormTrafficView::on_mbDisconnected);
    connect(&_mbMultiServer, &ModbusMultiServer::definitionsChanged, this, &FormTrafficView::on_mbDefinitionsChanged);
}

///
/// \brief FormTrafficView::addToolbarSpacer
/// \param width
///
void FormTrafficView::addToolbarSpacer(int width)
{
    auto* spacer = new QWidget(ui->toolBarTraffic);
    spacer->setFixedWidth(width);
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    ui->toolBarTraffic->addWidget(spacer);
}

///
/// \brief FormTrafficView::resetTrafficCounters
///
void FormTrafficView::resetTrafficCounters()
{
    _requestCount = 0;
    _responseCount = 0;
}

///
/// \brief FormTrafficView::clearTrafficLog
///
void FormTrafficView::clearTrafficLog()
{
    if (_logUiFlushTimer)
        _logUiFlushTimer->stop();
    _pendingLogViewUpdates.clear();
    _trafficBuffer.clear();
    ui->outputWidget->clearLogView();
    resetTrafficCounters();
    updateExportActionState();
}

///
/// \brief FormTrafficView::flushPendingTrafficUiAll
///
void FormTrafficView::flushPendingTrafficUiAll()
{
    if (_logUiFlushTimer)
        _logUiFlushTimer->stop();

    while (!_pendingLogViewUpdates.isEmpty()) {
        const int batchSize = qMin(TrafficUiFlushChunkSize, _pendingLogViewUpdates.size());
        QVector<QSharedPointer<const ModbusMessage>> batch;
        batch.reserve(batchSize);
        for (int i = 0; i < batchSize; ++i)
            batch.push_back(_pendingLogViewUpdates.dequeue());
        ui->outputWidget->updateTrafficBatch(batch);
    }

    updateExportActionState();
}

///
/// \brief FormTrafficView::scheduleTrafficUiFlush
///
void FormTrafficView::scheduleTrafficUiFlush()
{
    if (!_logUiFlushTimer || _pendingLogViewUpdates.isEmpty() || _logUiFlushTimer->isActive())
        return;

    _logUiFlushTimer->start();
}

///
/// \brief FormTrafficView::trimTrafficBufferToLimit
///
void FormTrafficView::trimTrafficBufferToLimit()
{
    const int limit = qMax(1, static_cast<int>(_displayDefinition.LogViewLimit));
    while(_trafficBuffer.size() > limit)
        _trafficBuffer.dequeue();
}

///
/// \brief FormTrafficView::rebuildVisibleTraffic
///
void FormTrafficView::rebuildVisibleTraffic()
{
    if (_logUiFlushTimer)
        _logUiFlushTimer->stop();
    _pendingLogViewUpdates.clear();

    const bool paused = (_logViewState == LogViewState::Paused);
    if(paused)
        ui->outputWidget->setLogViewState(LogViewState::Running);

    ui->outputWidget->clearLogView();
    resetTrafficCounters();
    QVector<QSharedPointer<const ModbusMessage>> visibleMessages;
    visibleMessages.reserve(_trafficBuffer.size());

    for(const auto& entry : _trafficBuffer)
    {
        if(!entry.DisplayMessage)
            continue;

        if(matchesTrafficFilter(entry.Connection, entry.FilterMessage, entry.DisplayMessage))
        {
            if(entry.IsRequest)
                ++_requestCount;
            else
                ++_responseCount;

            visibleMessages.push_back(entry.DisplayMessage);
        }
    }

    ui->outputWidget->updateTrafficBatch(visibleMessages);

    if(paused)
        ui->outputWidget->setLogViewState(LogViewState::Paused);

    updateExportActionState();
}

///
/// \brief FormTrafficView::appendTrafficEntry
///
void FormTrafficView::appendTrafficEntry(const ConnectionDetails& cd,
                                         const QSharedPointer<const ModbusMessage>& displayMessage,
                                         const QSharedPointer<const ModbusMessage>& filterMessage,
                                         bool isRequest)
{
    if(!isVisible() || !displayMessage)
        return;

    TrafficLogEntry entry;
    entry.Connection = cd;
    entry.DisplayMessage = displayMessage;
    entry.FilterMessage = filterMessage ? filterMessage : displayMessage;
    entry.IsRequest = isRequest;
    _trafficBuffer.enqueue(entry);
    trimTrafficBufferToLimit();

    if(matchesTrafficFilter(cd, entry.FilterMessage, entry.DisplayMessage))
    {
        if(isRequest)
            ++_requestCount;
        else
            ++_responseCount;

        _pendingLogViewUpdates.enqueue(displayMessage);
        scheduleTrafficUiFlush();
        updateExportActionState();
    }
}







