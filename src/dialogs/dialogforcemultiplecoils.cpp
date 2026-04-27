#include <QtMath>
#include <QLineEdit>
#include <QSignalBlocker>
#include "modbuslimits.h"
#include "modbusmultiserver.h"
#include "uiutils.h"
#include "formatutils.h"
#include "dialogforcemultiplecoils.h"
#include "ui_dialogforcemultiplecoils.h"

namespace {
AddressBase addressBase(const ModbusWriteParams& params)
{
    return params.ZeroBasedAddress ? AddressBase::Base0 : AddressBase::Base1;
}

constexpr int kBitPointLengthLimit = 2000;

QRange<int> lengthRangeForPointType(int address, bool zeroBased, AddressSpace space)
{
    const auto defaultRange = ModbusLimits::lengthRange(address, zeroBased, space);
    const int offset = address - (zeroBased ? 0 : 1);
    const int maxByAddress = ModbusLimits::addressSpaceSize(space) - offset;
    const int maxLen = qMin(kBitPointLengthLimit, maxByAddress);
    return { defaultRange.from(), qMax(defaultRange.from(), maxLen) };
}
}

///
/// \brief DialogForceMultipleCoils::DialogForceMultipleCoils
/// \param params
/// \param type
/// \param length
/// \param dd
/// \param parent
///
DialogForceMultipleCoils::DialogForceMultipleCoils(ModbusWriteParams& params, QModbusDataUnit::RegisterType type, int length, bool displayHexAddresses, QWidget *parent) :
      QAdjustedSizeDialog(parent)
    , ui(new Ui::DialogForceMultipleCoils)
    ,_writeParams(params)
    ,_type(type)
    ,_hexAddress(displayHexAddresses)
{
    ui->setupUi(this);

    switch(type)
    {
        case QModbusDataUnit::Coils:
            setWindowTitle(tr("FORCE MULTIPLE COILS (0x)"));
            break;
        case QModbusDataUnit::DiscreteInputs:
            setWindowTitle(tr("FORCE DISCRETE INPUTS (1x)"));
            break;
        default:
            break;
    }

    recolorPushButtonIcon(ui->pushButtonExport, Qt::red);
    recolorPushButtonIcon(ui->pushButtonImport, Qt::darkGreen);

    _data = params.Value.value<QVector<quint16>>();
    if(_data.length() != length)
        _data.resize(length);

    setupAddressControls(length);
    updateAddressSummary();
    updateTableWidget();
}

///
/// \brief DialogForceMultipleCoils::~DialogForceMultipleCoils
///
DialogForceMultipleCoils::~DialogForceMultipleCoils()
{
    delete ui;
}

///
/// rief DialogForceMultipleCoils::changeEvent
///
///
/// \brief DialogForceMultipleCoils::changeEvent
///
void DialogForceMultipleCoils::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        updateAddressSummary();
    }

    QDialog::changeEvent(event);
}

///
/// \brief DialogForceMultipleCoils::setupAddressControls
///
void DialogForceMultipleCoils::setupAddressControls(int length)
{
    ui->lineEditDeviceId->setLeadingZeroes(_writeParams.LeadingZeros);
    ui->lineEditDeviceId->setInputRange(ModbusLimits::slaveRange());
    ui->lineEditDeviceId->setValue(_writeParams.DeviceId);
    ui->lineEditDeviceId->setHexView(_hexAddress);
    ui->lineEditDeviceId->setHexButtonVisible(false);

    ui->lineEditAddress->setLeadingZeroes(_writeParams.LeadingZeros);
    ui->lineEditAddress->setInputMode(_hexAddress ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditAddress->setInputRange(ModbusLimits::addressRange(_writeParams.AddrSpace, _writeParams.ZeroBasedAddress));
    ui->lineEditAddress->setValue(_writeParams.Address);
    ui->lineEditAddress->setHexView(_hexAddress);
    ui->lineEditAddress->setHexButtonVisible(false);

    const auto initialLenRange = lengthRangeForPointType(_writeParams.Address, _writeParams.ZeroBasedAddress, _writeParams.AddrSpace);
    const int initialLength = qBound(initialLenRange.from(), length, initialLenRange.to());
    if(_data.size() != initialLength)
        _data.resize(initialLength);

    ui->lineEditLength->setLeadingZeroes(_writeParams.LeadingZeros);
    ui->lineEditLength->setInputMode(_hexAddress ? NumericLineEdit::HexMode : NumericLineEdit::Int32Mode);
    ui->lineEditLength->setInputRange(initialLenRange);
    ui->lineEditLength->setValue(initialLength);
    ui->lineEditLength->setHexView(_hexAddress);
    ui->lineEditLength->setHexButtonVisible(false);

    connect(ui->lineEditDeviceId,
            static_cast<void (NumericLineEdit::*)(const QVariant&)>(&NumericLineEdit::valueChanged),
            this,
            [this](const QVariant&) {
        _writeParams.DeviceId = ui->lineEditDeviceId->value<quint32>();
        reloadDataFromServer();
        updateTableWidget();
    });

    connect(ui->lineEditAddress,
            static_cast<void (NumericLineEdit::*)(const QVariant&)>(&NumericLineEdit::valueChanged),
            this,
            [this](const QVariant&) {
        const int address = ui->lineEditAddress->value<int>();
        _writeParams.Address = static_cast<quint16>(address);

        const auto lenRange = lengthRangeForPointType(address, _writeParams.ZeroBasedAddress, _writeParams.AddrSpace);
        QSignalBlocker lengthBlocker(ui->lineEditLength);
        ui->lineEditLength->setInputRange(lenRange);
        if(ui->lineEditLength->value<int>() > lenRange.to()) {
            ui->lineEditLength->setValue(lenRange.to());
            ui->lineEditLength->update();
        }

        reloadDataFromServer();
        updateAddressSummary();
        updateTableWidget();
    });

    connect(ui->lineEditLength,
            static_cast<void (NumericLineEdit::*)(const QVariant&)>(&NumericLineEdit::valueChanged),
            this,
            [this](const QVariant&) {
        reloadDataFromServer();
        updateAddressSummary();
        updateTableWidget();
    });
}

///
/// \brief DialogForceMultipleCoils::updateAddressSummary
///
void DialogForceMultipleCoils::updateAddressSummary()
{
    const int length = qMax(1, _data.size());
    ui->labelAddresses->setText(
        QString("<html><head/><body><p>%3<span style=\" font-weight:700;\">%1 </span>→ %4<span style=\" font-weight:700;\">%2</span></p></body></html>").arg(
        formatAddress(_type, _writeParams.Address, _writeParams.AddrSpace, _hexAddress, addressBase(_writeParams)),
        formatAddress(_type, _writeParams.Address + length - 1, _writeParams.AddrSpace, _hexAddress, addressBase(_writeParams)),
        tr("Starting Address: "), tr("Ending Address: ")));
}

///
/// \brief DialogForceMultipleCoils::reloadDataFromServer
///
void DialogForceMultipleCoils::reloadDataFromServer()
{
    const int length = ui->lineEditLength->value<int>();
    if(length <= 0) {
        _data.clear();
        return;
    }

    if(_writeParams.Server == nullptr) {
        _data.resize(length);
        return;
    }

    const int serverAddress = _writeParams.Address - (_writeParams.ZeroBasedAddress ? 0 : 1);
    const auto data = _writeParams.Server->data(
        static_cast<quint8>(_writeParams.DeviceId),
        _type,
        static_cast<quint16>(serverAddress),
        static_cast<quint16>(length));

    _data = data.values();
    if(_data.size() != length)
        _data.resize(length);
}

///
/// \brief DialogForceMultipleCoils::accept
///
void DialogForceMultipleCoils::accept()
{
    _writeParams.Value = QVariant::fromValue(_data);
    _writeParams.DeviceId = ui->lineEditDeviceId->value<quint32>();
    _writeParams.Address = ui->lineEditAddress->value<quint16>();
    QDialog::accept();
}

///
/// \brief DialogForceMultipleCoils::on_pushButton0_clicked
///
void DialogForceMultipleCoils::on_pushButton0_clicked()
{
    for(auto& v : _data)
    {
        v = 0;
    }

    updateTableWidget();
}

///
/// \brief DialogForceMultipleCoils::on_pushButton1_clicked
///
void DialogForceMultipleCoils::on_pushButton1_clicked()
{
    for(auto& v : _data)
    {
        v = 1;
    }

    updateTableWidget();
}

///
/// \brief DialogForceMultipleCoils::on_pushButtonRandom_clicked
///
void DialogForceMultipleCoils::on_pushButtonRandom_clicked()
{
    for(auto& v : _data)
    {
        v = (QRandomGenerator::global()->bounded(0, 2) != 0);
    }

    updateTableWidget();
}

///
/// \brief DialogForceMultipleCoils::on_pushButtonImport_clicked
///
void DialogForceMultipleCoils::on_pushButtonImport_clicked()
{
    auto filename = QFileDialog::getOpenFileName(this, QString(), QString(), tr("CSV files (*.csv)"));
    if(filename.isEmpty())
        return;

    QFile file(filename);
    if(!file.open(QFile::ReadOnly))
    {
        QMessageBox::critical(this, tr("Error"), file.errorString());
        return;
    }

    QTextStream ts(&file);

    QVector<quint16> newData;
    bool headerSkipped = false;

    while(!ts.atEnd())
    {
        QString line = ts.readLine().trimmed();
        if(line.isEmpty()) {
            continue;
        }

        if(!headerSkipped)
        {
            headerSkipped = true;
            continue;
        }

        const QStringList parts = line.split(";");
        if(parts.size() < 2) {
            continue;
        }

        const auto valueStr = parts[1].trimmed();

        bool ok = false;
        const quint16 value = (valueStr.toUShort(&ok, 10) != 0);

        if(!ok)
        {
            QMessageBox::warning(this, tr("Import error"), tr("Invalid value: %1").arg(valueStr));
            return;
        }

        newData.append(value);
    }

    if(newData.isEmpty())
    {
        QMessageBox::warning(this, tr("Warning"), tr("No data found in file."));
        return;
    }

    if(newData.size() != _data.size())
    {
        QMessageBox::warning(this, tr("Warning"), tr("Imported data size (%1) does not match current size (%2).").arg(newData.size()).arg(_data.size()));
    }

    for(int i = 0; i < _data.size(); ++i) {
        if(i < newData.size()) {
            _data[i] = newData[i];
        }
    }

    updateTableWidget();

}

///
/// \brief DialogForceMultipleCoils::on_pushButtonExport_clicked
///
void DialogForceMultipleCoils::on_pushButtonExport_clicked()
{
    auto filename = QFileDialog::getSaveFileName(this, QString(), QString(), tr("CSV files (*.csv)"));
    if(filename.isEmpty()) return;

    if(!filename.endsWith(".csv", Qt::CaseInsensitive))
    {
        filename += ".csv";
    }

    QFile file(filename);
    if(!file.open(QFile::WriteOnly))
    {
        QMessageBox::critical(this, tr("Error"), file.errorString());
        return;
    }

    QTextStream ts(&file);
    ts.setGenerateByteOrderMark(true);

    const char* delim = ";";
    ts << "Address" << delim << "Value" << "\n";

    for(int i = 0; i < _data.size(); i++)
    {
        ts << formatAddress(_type, _writeParams.Address + i, _writeParams.AddrSpace, _hexAddress, addressBase(_writeParams))
        << delim
///
/// \brief QString::number
///
        << QString::number(_data[i])
        << "\n";
    }
}

///
/// \brief DialogForceMultipleCoils::on_tableWidget_itemDoubleClicked
/// \param item
///
void DialogForceMultipleCoils::on_tableWidget_itemDoubleClicked(QTableWidgetItem* item)
{
    if(item == nullptr) return;

    bool ok;
    const auto idx = item->data(Qt::UserRole).toInt(&ok);
    if(ok)
    {
        _data[idx] = !_data[idx];
        item->setText(QString::number(_data[idx]));
    }
}

///
/// \brief DialogForceMultipleCoils::updateTableWidget
///
void DialogForceMultipleCoils::updateTableWidget()
{
    const int columns = 8;
    const auto length = _data.length();

    ui->tableWidget->clear();
    ui->tableWidget->setColumnCount(columns);
    ui->tableWidget->setRowCount(qCeil(length / (double)columns));

    for(int i = 0; i < ui->tableWidget->columnCount(); i++)
    {
        const auto text = QString("+%1").arg(i);
        ui->tableWidget->setHorizontalHeaderItem(i, new QTableWidgetItem(text));
    }

    for(int i = 0; i < ui->tableWidget->rowCount(); i++)
    {
        const auto addressFrom = formatAddress(_type, _writeParams.Address + i * columns, _writeParams.AddrSpace, _hexAddress, addressBase(_writeParams));
        const auto addressTo = formatAddress(_type, _writeParams.Address + qMin(length - 1, (i + 1) * columns - 1), _writeParams.AddrSpace, _hexAddress, addressBase(_writeParams));
        ui->tableWidget->setVerticalHeaderItem(i, new QTableWidgetItem(QString("%1-%2").arg(addressFrom, addressTo)));

        for(int j = 0; j < columns; j++)
        {
            const auto idx = i * columns + j;
            if(idx < length)
            {
                auto item = new QTableWidgetItem(QString::number(_data[idx]));
                item->setData(Qt::UserRole, idx);
                item->setTextAlignment(Qt::AlignCenter);
                item->setToolTip(formatAddress(_type,_writeParams.Address + idx, _writeParams.AddrSpace, _hexAddress, addressBase(_writeParams)));
                ui->tableWidget->setItem(i, j, item);
            }
            else
            {
                auto lineEdit = new QLineEdit(ui->tableWidget);
                lineEdit->setText("-");
                lineEdit->setFrame(false);
                lineEdit->setMaximumWidth(40);
                lineEdit->setEnabled(false);
                lineEdit->setAlignment(Qt::AlignCenter);
                ui->tableWidget->setCellWidget(i, j, lineEdit);
            }
        }
    }
    ui->tableWidget->resizeColumnsToContents();
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}

