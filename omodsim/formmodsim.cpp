#include <QPainter>
#include <QPalette>
#include <QDateTime>
#include "modbuslimits.h"
#include "mainwindow.h"
#include "dialogwritecoilregister.h"
#include "dialogwriteholdingregister.h"
#include "dialogwriteholdingregisterbits.h"
#include "formmodsim.h"
#include "ui_formmodsim.h"

///
/// \brief FormModSim::FormModSim
/// \param num
/// \param parent
///
FormModSim::FormModSim(int id, ModbusMultiServer& server, MainWindow* parent) :
    QWidget(parent)
    , ui(new Ui::FormModSim)
    ,_formId(id)
    ,_mbMultiServer(server)
{
    Q_ASSERT(parent != nullptr);

    ui->setupUi(this);

    _timer.setInterval(1000);
    setWindowTitle(QString("ModSim%1").arg(_formId));

    ui->lineEditAddress->setPaddingZeroes(true);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange());
    ui->lineEditAddress->setValue(1);

    ui->lineEditLength->setInputRange(ModbusLimits::lengthRange());
    ui->lineEditLength->setValue(100);

    ui->lineEditDeviceId->setInputRange(ModbusLimits::slaveRange());
    ui->lineEditDeviceId->setValue(1);

    ui->comboBoxModbusPointType->setCurrentPointType(QModbusDataUnit::HoldingRegisters);

    {
        const auto dd = displayDefinition();

        _mbMultiServer.setDeviceId(dd.DeviceId);
        _mbMultiServer.addUnitMap(formId(), dd.PointType, dd.PointAddress - 1, dd.Length);

        ui->outputWidget->setup(dd);
        ui->outputWidget->setFocus();
    }

    connect(&_mbMultiServer, &ModbusMultiServer::request, this, &FormModSim::on_mbRequest);
    connect(&_mbMultiServer, &ModbusMultiServer::response, this, &FormModSim::on_mbResponse);
    connect(&_mbMultiServer, &ModbusMultiServer::connected, this, &FormModSim::on_mbConnected);
    connect(&_mbMultiServer, &ModbusMultiServer::disconnected, this, &FormModSim::on_mbDisconnected);
    connect(&_mbMultiServer, &ModbusMultiServer::deviceIdChanged, this, &FormModSim::on_mbDeviceIdChanged);

    connect(&_timer, &QTimer::timeout, this, &FormModSim::on_timeout);
    _timer.start();
}

///
/// \brief FormModSim::~FormModSim
///
FormModSim::~FormModSim()
{
    _mbMultiServer.removeUnitMap(formId());
    delete ui;
}

///
/// \brief FormModSim::changeEvent
/// \param event
///
void FormModSim::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        if(!_mbMultiServer.isConnected())
            ui->outputWidget->setStatus(tr("NOT CONNECTED!"));
    }

    QWidget::changeEvent(event);
}

///
/// \brief FormModSim::closeEvent
/// \param event
///
void FormModSim::closeEvent(QCloseEvent *event)
{
    emit closing();
    QWidget::closeEvent(event);
}

///
/// \brief FormModSim::filename
/// \return
///
QString FormModSim::filename() const
{
    return _filename;
}

///
/// \brief FormModSim::setFilename
/// \param filename
///
void FormModSim::setFilename(const QString& filename)
{
    _filename = filename;
}

///
/// \brief FormModSim::displayDefinition
/// \return
///
DisplayDefinition FormModSim::displayDefinition() const
{
    DisplayDefinition dd;
    dd.UpdateRate = _timer.interval();
    dd.DeviceId = ui->lineEditDeviceId->value<int>();
    dd.PointAddress = ui->lineEditAddress->value<int>();
    dd.PointType = ui->comboBoxModbusPointType->currentPointType();
    dd.Length = ui->lineEditLength->value<int>();

    return dd;
}

///
/// \brief FormModSim::setDisplayDefinition
/// \param dd
///
void FormModSim::setDisplayDefinition(const DisplayDefinition& dd)
{
    _timer.setInterval(qBound(20U, dd.UpdateRate, 10000U));
    ui->lineEditDeviceId->setValue(dd.DeviceId);
    ui->lineEditAddress->setValue(dd.PointAddress);
    ui->lineEditLength->setValue(dd.Length);
    ui->comboBoxModbusPointType->setCurrentPointType(dd.PointType);

    ui->outputWidget->setup(dd);
}

///
/// \brief FormModSim::displayMode
/// \return
///
DisplayMode FormModSim::displayMode() const
{
    return ui->outputWidget->displayMode();
}

///
/// \brief FormModSim::setDisplayMode
/// \param mode
///
void FormModSim::setDisplayMode(DisplayMode mode)
{
    ui->outputWidget->setDisplayMode(mode);
}

///
/// \brief FormModSim::dataDisplayMode
/// \return
///
DataDisplayMode FormModSim::dataDisplayMode() const
{
    return ui->outputWidget->dataDisplayMode();
}

///
/// \brief FormModSim::displayHexAddresses
/// \return
///
bool FormModSim::displayHexAddresses() const
{
    return ui->outputWidget->displayHexAddresses();
}

///
/// \brief FormModSim::setDisplayHexAddresses
/// \param on
///
void FormModSim::setDisplayHexAddresses(bool on)
{
    ui->outputWidget->setDisplayHexAddresses(on);
}

///
/// \brief FormModSim::setDataDisplayMode
/// \param mode
///
void FormModSim::setDataDisplayMode(DataDisplayMode mode)
{
    ui->outputWidget->setDataDisplayMode(mode);
}

///
/// \brief FormModSim::backgroundColor
/// \return
///
QColor FormModSim::backgroundColor() const
{
    return ui->outputWidget->backgroundColor();
}

///
/// \brief FormModSim::setBackgroundColor
/// \param clr
///
void FormModSim::setBackgroundColor(const QColor& clr)
{
    ui->outputWidget->setBackgroundColor(clr);
}

///
/// \brief FormModSim::foregroundColor
/// \return
///
QColor FormModSim::foregroundColor() const
{
    return ui->outputWidget->foregroundColor();
}

///
/// \brief FormModSim::setForegroundColor
/// \param clr
///
void FormModSim::setForegroundColor(const QColor& clr)
{
    ui->outputWidget->setForegroundColor(clr);
}

///
/// \brief FormModSim::statusColor
/// \return
///
QColor FormModSim::statusColor() const
{
    return ui->outputWidget->statusColor();
}

///
/// \brief FormModSim::setStatusColor
/// \param clr
///
void FormModSim::setStatusColor(const QColor& clr)
{
    ui->outputWidget->setStatusColor(clr);
}

///
/// \brief FormModSim::font
/// \return
///
QFont FormModSim::font() const
{
   return ui->outputWidget->font();
}

///
/// \brief FormModSim::setFont
/// \param font
///
void FormModSim::setFont(const QFont& font)
{
    ui->outputWidget->setFont(font);
}

///
/// \brief FormModSim::print
/// \param printer
///
void FormModSim::print(QPrinter* printer)
{
    if(!printer) return;

    auto layout = printer->pageLayout();
    const auto resolution = printer->resolution();
    auto pageRect = layout.paintRectPixels(resolution);

    const auto margin = qMax(pageRect.width(), pageRect.height()) * 0.05;
    layout.setMargins(QMargins(margin, margin, margin, margin));
    pageRect.adjust(layout.margins().left(), layout.margins().top(), -layout.margins().right(), -layout.margins().bottom());

    const int pageWidth = pageRect.width();
    const int pageHeight = pageRect.height();

    const int cx = pageRect.left();
    const int cy = pageRect.top();

    QPainter painter(printer);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    const auto textTime = QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat);
    auto rcTime = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextSingleLine, textTime);

    const auto textDevId = QString(tr("Device Id: %1")).arg(ui->lineEditDeviceId->text());
    auto rcDevId = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextSingleLine, textDevId);

    const auto textAddrLen = QString(tr("Address: %1\nLength: %2")).arg(ui->lineEditAddress->text(), ui->lineEditLength->text());
    auto rcAddrLen = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextWordWrap, textAddrLen);

    const auto textType = QString(tr("MODBUS Point Type:\n%1")).arg(ui->comboBoxModbusPointType->currentText());
    auto rcType = painter.boundingRect(cx, cy, pageWidth, pageHeight, Qt::TextWordWrap, textType);

    rcTime.moveTopRight({ pageRect.right(), 10 });
    rcDevId.moveLeft(rcAddrLen.right() + 40);
    rcAddrLen.moveTop(rcDevId.bottom() + 10);
    rcType.moveTopLeft({ rcDevId.left(), rcAddrLen.top() });

    painter.drawText(rcTime, Qt::TextSingleLine, textTime);
    painter.drawText(rcDevId, Qt::TextSingleLine, textDevId);
    painter.drawText(rcAddrLen, Qt::TextWordWrap, textAddrLen);
    painter.drawText(rcType, Qt::TextWordWrap, textType);

    auto rcOutput = pageRect;
    rcOutput.setTop(rcAddrLen.bottom() + 20);

    ui->outputWidget->paint(rcOutput, painter);
}

///
/// \brief FormModSim::show
///
void FormModSim::show()
{
    QWidget::show();
    emit showed();
}

///
/// \brief FormModSim::on_timeout
///
void FormModSim::on_timeout()
{
    if(!_mbMultiServer.isConnected())
    {
        ui->outputWidget->setStatus(tr("NOT CONNECTED!"));
        return;
    }

    const auto dd = displayDefinition();
    if(dd.PointAddress + dd.Length - 1 > ModbusLimits::addressRange().to())
    {
        ui->outputWidget->setStatus(tr("Invalid Data Length Specified"));
        return;
    }

    ui->outputWidget->setStatus(QString());
    ui->outputWidget->updateData(_mbMultiServer.data(dd.PointType, dd.PointAddress - 1, dd.Length));
}

///
/// \brief FormModSim::on_lineEditAddress_valueChanged
///
void FormModSim::on_lineEditAddress_valueChanged(const QVariant&)
{
    const auto dd = displayDefinition();
    ui->outputWidget->setup(dd);

    _mbMultiServer.addUnitMap(formId(), dd.PointType, dd.PointAddress - 1, dd.Length);
}

///
/// \brief FormModSim::on_lineEditLength_valueChanged
///
void FormModSim::on_lineEditLength_valueChanged(const QVariant&)
{
    const auto dd = displayDefinition();
    ui->outputWidget->setup(dd);

    _mbMultiServer.addUnitMap(formId(), dd.PointType, dd.PointAddress - 1, dd.Length);
}

///
/// \brief FormModSim::on_lineEditDeviceId_valueChanged
///
void FormModSim::on_lineEditDeviceId_valueChanged(const QVariant&)
{
    const auto deviceId = ui->lineEditDeviceId->value<int>();
    _mbMultiServer.setDeviceId(deviceId);
}

///
/// \brief FormModSim::on_comboBoxModbusPointType_pointTypeChanged
/// \param value
///
void FormModSim::on_comboBoxModbusPointType_pointTypeChanged(QModbusDataUnit::RegisterType value)
{
    const auto dd = displayDefinition();
    ui->outputWidget->setup(dd);

    _mbMultiServer.addUnitMap(formId(), value, dd.PointAddress - 1, dd.Length);
}

///
/// \brief FormModSim::on_outputWidget_itemDoubleClicked
/// \param addr
/// \param value
///
void FormModSim::on_outputWidget_itemDoubleClicked(quint16 addr, const QVariant& value)
{
    if(!_mbMultiServer.isConnected())
        return;

    const auto mode = dataDisplayMode();
    const auto pointType = ui->comboBoxModbusPointType->currentPointType();
    auto& simParams = _simulationMap[{pointType, addr}];

    switch(pointType)
    {
        case QModbusDataUnit::Coils:
        case QModbusDataUnit::DiscreteInputs:
        {
            ModbusWriteParams params = { addr, value, mode };
            DialogWriteCoilRegister dlg(params, simParams, this);
            if(dlg.exec() == QDialog::Accepted)
            {
                if(simParams.Mode == SimulationMode::No) _mbMultiServer.writeRegister(pointType, params);
                _mbMultiServer.simulateRegister(mode, pointType, addr, simParams);
            }
        }
        break;

        case QModbusDataUnit::InputRegisters:
        case QModbusDataUnit::HoldingRegisters:
        {
            ModbusWriteParams params = { addr, value, mode };
            if(mode == DataDisplayMode::Binary)
            {
                DialogWriteHoldingRegisterBits dlg(params, this);
                if(dlg.exec() == QDialog::Accepted)
                    _mbMultiServer.writeRegister(pointType, params);
            }
            else
            {
                DialogWriteHoldingRegister dlg(params, simParams, this);
                if(dlg.exec() == QDialog::Accepted)
                {
                    if(simParams.Mode == SimulationMode::No) _mbMultiServer.writeRegister(pointType, params);
                    _mbMultiServer.simulateRegister(mode, pointType, addr, simParams);
                }
            }
        }
        break;

        default:
        break;
    }
}

///
/// \brief FormModSim::on_mbDeviceIdChanged
/// \param deviceId
///
void FormModSim::on_mbDeviceIdChanged(quint8 deviceId)
{
    blockSignals(true);
    ui->lineEditDeviceId->setValue(deviceId);
    blockSignals(false);
}

///
/// \brief FormModSim::on_mbConnected
///
void FormModSim::on_mbConnected(const ConnectionDetails&)
{
}

///
/// \brief FormModSim::on_mbDisconnected
///
void FormModSim::on_mbDisconnected(const ConnectionDetails&)
{
    if(!_mbMultiServer.isConnected())
        _simulationMap.clear();
}

///
/// \brief FormModSim::on_mbRequest
/// \param req
///
void FormModSim::on_mbRequest(const QModbusRequest& req)
{
    const auto deviceId = ui->lineEditDeviceId->value<int>();
    ui->outputWidget->updateTraffic(req, deviceId);
}

///
/// \brief FormModSim::on_mbResponse
/// \param resp
///
void FormModSim::on_mbResponse(const QModbusResponse& resp)
{
    const auto deviceId = ui->lineEditDeviceId->value<int>();
    ui->outputWidget->updateTraffic(resp, deviceId);
}
