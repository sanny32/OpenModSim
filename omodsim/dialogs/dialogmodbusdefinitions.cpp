#include <QPushButton>
#include "dialogmodbusdefinitions.h"
#include "ui_dialogmodbusdefinitions.h"

///
/// \brief DialogModbusDefinitions::DialogModbusDefinitions
/// \param srv
/// \param parent
///
DialogModbusDefinitions::DialogModbusDefinitions(ModbusMultiServer& srv, QWidget *parent)
    : QFixedSizeDialog(parent)
    , ui(new Ui::DialogModbusDefinitions)
    ,_mbMultiServer(srv)
{
    ui->setupUi(this);

    const QList<QCheckBox*> allCheckBoxes = findChildren<QCheckBox*>();
    for (QCheckBox* checkBox : allCheckBoxes) {
        #if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
            connect(checkBox, &QCheckBox::checkStateChanged, this, [this] {
                ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
                ui->spinBoxDelay->setEnabled(ui->checkBoxDelay->isChecked());
                ui->spinBoxUpToTime->setEnabled(ui->checkBoxRandomDelay->isChecked());
            });
        #else
            connect(checkBox, &QCheckBox::stateChanged, this, [this] {
                ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
                ui->spinBoxDelay->setEnabled(ui->checkBoxDelay->isChecked());
                ui->spinBoxUpToTime->setEnabled(ui->checkBoxRandomDelay->isChecked());
            });
        #endif
    }

    const QList<QSpinBox*> allSpinBoxes = findChildren<QSpinBox*>();
    for (QSpinBox* spinBox : allSpinBoxes) {
        connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this] {
            ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
        });
    }

    const QList<QComboBox*> allComboBoxes = findChildren<QComboBox*>();
    for (QComboBox* comboBox : allComboBoxes) {
        connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
        });
    }

    const auto defs = _mbMultiServer.getModbusDefinitions();
    updateModbusDefinitions(defs);
}


///
/// \brief DialogModbusDefinitions::~DialogModbusDefinitions
///
DialogModbusDefinitions::~DialogModbusDefinitions()
{
    delete ui;
}

///
/// \brief DialogModbusDefinitions::accept
///
void DialogModbusDefinitions::accept()
{
    apply();
    QFixedSizeDialog::accept();
}

///
/// \brief DialogModbusDefinitions::on_buttonBox_clicked
/// \param btn
///
void DialogModbusDefinitions::on_buttonBox_clicked(QAbstractButton* btn)
{
    if(ui->buttonBox->buttonRole(btn) == QDialogButtonBox::ApplyRole) {
        apply();
    }
}

///
/// \brief DialogModbusDefinitions::apply
///
void DialogModbusDefinitions::apply()
{
    ModbusDefinitions defs;
    defs.AddrSpace = ui->comboBoxAddrSpace->currentAddressSpace();
    defs.UseGlobalUnitMap = ui->checkBoxGlobalMap->isChecked();
    defs.ErrorSimulations.setResponseIncorrectId(ui->checkBoxErrIncorrentId->isChecked());
    defs.ErrorSimulations.setResponseIllegalFunction(ui->checkBoxIllegalFunc->isChecked());
    defs.ErrorSimulations.setResponseDeviceBusy(ui->checkBoxBusy->isChecked());
    defs.ErrorSimulations.setResponseIncorrectCrc(ui->checkBoxCrcErr->isChecked());
    defs.ErrorSimulations.setResponseDelay(ui->checkBoxDelay->isChecked());
    defs.ErrorSimulations.setResponseDelayTime(ui->spinBoxDelay->value());
    defs.ErrorSimulations.setResponseRandomDelay(ui->checkBoxRandomDelay->isChecked());
    defs.ErrorSimulations.setResponseRandomDelayUpToTime(ui->spinBoxUpToTime->value());
    defs.ErrorSimulations.setNoResponse(ui->checkBoxNoResponse->isChecked());

    _mbMultiServer.setModbusDefinitions(defs);

    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

///
/// \brief DialogModbusDefinitions::updateModbusDefinitions
/// \param defs
///
void DialogModbusDefinitions::updateModbusDefinitions(const ModbusDefinitions& defs)
{
    ui->checkBoxGlobalMap->setChecked(defs.UseGlobalUnitMap);
    ui->comboBoxAddrSpace->setCurrentAddressSpace(defs.AddrSpace);

    ui->checkBoxErrIncorrentId->setChecked(defs.ErrorSimulations.responseIncorrectId());
    ui->checkBoxIllegalFunc->setChecked(defs.ErrorSimulations.responseIllegalFunction());
    ui->checkBoxBusy->setChecked(defs.ErrorSimulations.responseDeviceBusy());
    ui->checkBoxCrcErr->setChecked(defs.ErrorSimulations.responseIncorrectCrc());
    ui->checkBoxDelay->setChecked(defs.ErrorSimulations.responseDelay());
    ui->checkBoxRandomDelay->setChecked(defs.ErrorSimulations.responseRandomDelay());
    ui->checkBoxNoResponse->setChecked(defs.ErrorSimulations.noResponse());

    ui->spinBoxDelay->setValue(defs.ErrorSimulations.responseDelayTime());
    ui->spinBoxDelay->setEnabled(defs.ErrorSimulations.responseDelay());
    ui->spinBoxUpToTime->setValue(defs.ErrorSimulations.responseRandomDelayUpToTime());
    ui->spinBoxUpToTime->setEnabled(defs.ErrorSimulations.responseRandomDelay());

    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}
