#include "modbuslimits.h"
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
    ui->lineEditPointAddress->setInputMode(dd.HexAddress ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditPointAddress->setInputRange(ModbusLimits::addressRange(dd.ZeroBasedAddress));
    ui->lineEditLength->setInputRange(ModbusLimits::lengthRange());
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

    ui->checkBoxGlobalMap->setChecked(dd.UseGlobalUnitMap);

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
/// \brief DialogDisplayDefinition::accept
///
void DialogDisplayDefinition::accept()
{
    _displayDefinition.DeviceId = ui->lineEditSlaveAddress->value<int>();
    _displayDefinition.PointAddress = ui->lineEditPointAddress->value<int>();
    _displayDefinition.PointType = ui->comboBoxPointType->currentPointType();
    _displayDefinition.Length = ui->lineEditLength->value<int>();
    _displayDefinition.LogViewLimit = ui->lineEditLogLimit->value<int>();
    _displayDefinition.AutoscrollLog = ui->checkBoxAutoscrollLog->isChecked();
    _displayDefinition.VerboseLogging = ui->checkBoxVerboseLogging->isChecked();
    _displayDefinition.ZeroBasedAddress = (ui->comboBoxAddressBase->currentAddressBase() == AddressBase::Base0);
    _displayDefinition.UseGlobalUnitMap = ui->checkBoxGlobalMap->isChecked();

    QFixedSizeDialog::accept();
}

///
/// \brief DialogDisplayDefinition::on_comboBoxAddressBase_currentIndexChanged
/// \param index
///
void DialogDisplayDefinition::on_comboBoxAddressBase_addressBaseChanged(AddressBase base)
{
    const auto addr = ui->lineEditPointAddress->value<int>();

    ui->lineEditPointAddress->setInputRange(ModbusLimits::addressRange(base == AddressBase::Base0));
    ui->lineEditPointAddress->setValue(base == AddressBase::Base1 ? qMax(1, addr + 1) : qMax(0, addr - 1));
}
