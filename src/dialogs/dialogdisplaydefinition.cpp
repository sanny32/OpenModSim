#include "modbuslimits.h"
#include "quintvalidator.h"
#include "displaydefinition.h"
#include "dialogdisplaydefinition.h"
#include "ui_dialogdisplaydefinition.h"

///
/// \brief DialogDisplayDefinition::DialogDisplayDefinition
/// \param dd
/// \param parent
///
DialogDisplayDefinition::DialogDisplayDefinition(DisplayDefinition dd, QWidget* parent)
    : QFixedSizeDialog(parent)
    ,_displayDefinition(dd)
    , ui(new Ui::DialogDisplayDefinition)
{
    ui->setupUi(this);
    ui->groupBoxScriptSettings->installEventFilter(this);

    ui->lineEditFormName->setText(dd.FormName);

    ui->comboBoxColumnsDistance->setValidator(new QUIntValidator(1, 32, this));
    ui->comboBoxColumnsDistance->setEditText(QString::number(dd.DataViewColumnsDistance));
    ui->checkBoxLeadingZeros->setChecked(dd.LeadingZeros);

    ui->lineEditPointAddress->setInputMode(dd.HexAddress ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditPointAddress->setInputRange(ModbusLimits::addressRange(dd.AddrSpace, dd.ZeroBasedAddress));
    ui->lineEditLength->setInputRange(ModbusLimits::lengthRange(dd.PointAddress, dd.ZeroBasedAddress, dd.AddrSpace));
    ui->lineEditSlaveAddress->setInputRange(ModbusLimits::slaveRange());
    ui->lineEditLogLimit->setInputRange(4, 1000);
    ui->checkBoxAutoscrollLog->setChecked(dd.AutoscrollLog);
    ui->checkBoxVerboseLogging->setChecked(dd.VerboseLogging);

    ui->comboBoxAddressBase->setCurrentIndex(dd.ZeroBasedAddress ? 0 : 1);
    ui->lineEditPointAddress->setValue(dd.PointAddress);
    ui->lineEditSlaveAddress->setValue(dd.DeviceId);
    ui->lineEditLength->setValue(dd.Length);
    ui->lineEditLogLimit->setValue(dd.LogViewLimit);
    ui->comboBoxPointType->setCurrentPointType(dd.PointType);

    ui->lineEditScriptInterval->setValue(dd.ScriptCfg.Interval);
    ui->comboBoxScriptRunMode->setCurrentRunMode(dd.ScriptCfg.Mode);
    ui->checkBoxRunOnStartup->setChecked(dd.ScriptCfg.RunOnStartup);

    ui->buttonBox->setFocus();
}

///
/// \brief DialogDisplayDefinition::~DialogDisplayDefinition
///
DialogDisplayDefinition::~DialogDisplayDefinition()
{
    delete ui;
}

///
/// \brief DialogDisplayDefinition::eventFilter
/// \param obj
/// \param event
/// \return
///
bool DialogDisplayDefinition::eventFilter(QObject* obj, QEvent* event)
{
    if(event->type() == QEvent::Paint)
    {
        QGroupBox* box = qobject_cast<QGroupBox*>(obj);
        if(box && box->objectName() == "groupBoxScriptSettings")
        {
            static QPixmap pixmap(":/res/banner-js.png");

            QPainter painter(box);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            const QSize pixSize = pixmap.size();
            const int x = box->width() - pixSize.width() - 15;
            const int y = (box->height() - pixSize.height()) / 2 + 5;

            painter.setOpacity(0.5);
            painter.drawPixmap(x, y, pixmap);
        }
    }

    return QFixedSizeDialog::eventFilter(obj, event);
}

///
/// \brief DialogDisplayDefinition::accept
///
void DialogDisplayDefinition::accept()
{
    _displayDefinition.FormName = ui->lineEditFormName->text();
    _displayDefinition.DeviceId = ui->lineEditSlaveAddress->value<int>();
    _displayDefinition.PointAddress = ui->lineEditPointAddress->value<int>();
    _displayDefinition.PointType = ui->comboBoxPointType->currentPointType();
    _displayDefinition.Length = ui->lineEditLength->value<int>();
    _displayDefinition.LogViewLimit = ui->lineEditLogLimit->value<int>();
    _displayDefinition.AutoscrollLog = ui->checkBoxAutoscrollLog->isChecked();
    _displayDefinition.VerboseLogging = ui->checkBoxVerboseLogging->isChecked();
    _displayDefinition.ZeroBasedAddress = (ui->comboBoxAddressBase->currentAddressBase() == AddressBase::Base0);
    _displayDefinition.DataViewColumnsDistance = ui->comboBoxColumnsDistance->currentText().toUInt();
    _displayDefinition.LeadingZeros = ui->checkBoxLeadingZeros->isChecked();
    _displayDefinition.ScriptCfg.Interval = ui->lineEditScriptInterval->value<int>();
    _displayDefinition.ScriptCfg.Mode = ui->comboBoxScriptRunMode->currentRunMode();
    _displayDefinition.ScriptCfg.RunOnStartup = ui->checkBoxRunOnStartup->isChecked();

    QFixedSizeDialog::accept();
}

///
/// \brief DialogDisplayDefinition::on_comboBoxAddressBase_currentIndexChanged
/// \param index
///
void DialogDisplayDefinition::on_lineEditPointAddress_valueChanged(const QVariant&)
{
    const auto addrSpace = _displayDefinition.AddrSpace;
    const bool zeroBased = (ui->comboBoxAddressBase->currentAddressBase() == AddressBase::Base0);
    const int address = ui->lineEditPointAddress->value<int>();
    const auto lenRange = ModbusLimits::lengthRange(address, zeroBased, addrSpace);

    ui->lineEditLength->setInputRange(lenRange);
    if(ui->lineEditLength->value<int>() > lenRange.to()) {
        ui->lineEditLength->setValue(lenRange.to());
        ui->lineEditLength->update();
    }
}

///
/// \brief DialogDisplayDefinition::on_lineEditLength_valueChanged
///
void DialogDisplayDefinition::on_lineEditLength_valueChanged(const QVariant&)
{
}

///
/// \brief DialogDisplayDefinition::on_comboBoxAddressBase_addressBaseChanged
/// \param base
///
void DialogDisplayDefinition::on_comboBoxAddressBase_addressBaseChanged(AddressBase base)
{
    const auto addrSpace = _displayDefinition.AddrSpace;
    const bool zeroBased = (base == AddressBase::Base0);
    const auto addr = ui->lineEditPointAddress->value<int>();

    ui->lineEditPointAddress->setInputRange(ModbusLimits::addressRange(addrSpace, zeroBased));
    ui->lineEditPointAddress->setValue(base == AddressBase::Base1 ? qMax(1, addr + 1) : qMax(0, addr - 1));
    const int newAddr = ui->lineEditPointAddress->value<int>();
    const auto lenRange = ModbusLimits::lengthRange(newAddr, zeroBased, addrSpace);
    ui->lineEditLength->setInputRange(lenRange);
    if(ui->lineEditLength->value<int>() > lenRange.to())
        ui->lineEditLength->setValue(lenRange.to());
}
