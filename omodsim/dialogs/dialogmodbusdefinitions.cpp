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
        #if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
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
        connect(spinBox, &QSpinBox::valueChanged, this, [this] {
            ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
        });
    }

    const QList<QComboBox*> allComboBoxes = findChildren<QComboBox*>();
    for (QComboBox* comboBox : allComboBoxes) {
        connect(comboBox, &QComboBox::currentIndexChanged, this, [this] {
            ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
        });
    }

    const auto md = _mbMultiServer.getModbusDefinitions();
    updateModbusDefinitions(md);
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
    ModbusDefinitions md;
    md.AddrSpace = ui->comboBoxAddrSpace->currentAddressSpace();
    md.UseGlobalUnitMap = ui->checkBoxGlobalMap->isChecked();
    md.ErrorSimulations.setResponseIncorrectId(ui->checkBoxErrIncorrentId->isChecked());
    md.ErrorSimulations.setResponseIllegalFunction(ui->checkBoxIllegalFunc->isChecked());
    md.ErrorSimulations.setResponseDeviceBusy(ui->checkBoxBusy->isChecked());
    md.ErrorSimulations.setResponseIncorrectCrc(ui->checkBoxCrcErr->isChecked());
    md.ErrorSimulations.setResponseDelay(ui->checkBoxDelay->isChecked());
    md.ErrorSimulations.setResponseDelayTime(ui->spinBoxDelay->value());
    md.ErrorSimulations.setResponseRandomDelay(ui->checkBoxRandomDelay->isChecked());
    md.ErrorSimulations.setResponseRandomDelayUpToTime(ui->spinBoxUpToTime->value());
    md.ErrorSimulations.setNoResponse(ui->checkBoxNoResponse->isChecked());

    _mbMultiServer.setModbusDefinitions(md);

    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

///
/// \brief DialogModbusDefinitions::updateModbusDefinitions
/// \param md
///
void DialogModbusDefinitions::updateModbusDefinitions(const ModbusDefinitions& md)
{
    ui->checkBoxGlobalMap->setChecked(md.UseGlobalUnitMap);
    ui->comboBoxAddrSpace->setCurrentAddressSpace(md.AddrSpace);

    ui->checkBoxErrIncorrentId->setChecked(md.ErrorSimulations.responseIncorrectId());
    ui->checkBoxIllegalFunc->setChecked(md.ErrorSimulations.responseIllegalFunction());
    ui->checkBoxBusy->setChecked(md.ErrorSimulations.responseDeviceBusy());
    ui->checkBoxCrcErr->setChecked(md.ErrorSimulations.responseIncorrectCrc());
    ui->checkBoxDelay->setChecked(md.ErrorSimulations.responseDelay());
    ui->checkBoxRandomDelay->setChecked(md.ErrorSimulations.responseRandomDelay());
    ui->checkBoxNoResponse->setChecked(md.ErrorSimulations.noResponse());

    ui->spinBoxDelay->setValue(md.ErrorSimulations.responseDelayTime());
    ui->spinBoxDelay->setEnabled(md.ErrorSimulations.responseDelay());
    ui->spinBoxUpToTime->setValue(md.ErrorSimulations.responseRandomDelayUpToTime());
    ui->spinBoxUpToTime->setEnabled(md.ErrorSimulations.responseRandomDelay());

    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}
