#ifndef DIALOGFORCEMULTIPLEREGISTERS_H
#define DIALOGFORCEMULTIPLEREGISTERS_H

#include <QDialog>
#include <QVariant>
#include <QTableWidgetItem>
#include <QModbusDataUnit>
#include "qadjustedsizedialog.h"
#include "numericlineedit.h"
#include "modbuswriteparams.h"
#include "displaydefinition.h"

namespace Ui {
class DialogForceMultipleRegisters;
}
///
/// \brief The DialogForceMultipleRegisters class
///
class DialogForceMultipleRegisters : public QAdjustedSizeDialog
{
    Q_OBJECT

public:
    explicit DialogForceMultipleRegisters(ModbusWriteParams& params, QModbusDataUnit::RegisterType type, int length, bool displayHexAddresses, QWidget *parent = nullptr);
    ~DialogForceMultipleRegisters();

    void accept() override;

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_pushButton0_clicked();
    void on_pushButtonRandom_clicked();
    void on_pushButtonValue_clicked();
    void on_pushButtonInc_clicked();
    void on_pushButtonImport_clicked();
    void on_pushButtonExport_clicked();

private:
    void setupAddressControls(int length);
    void setupDisplayControls();
    void setupPresetData();
    void setupEditorInputs();

    void updateDisplayBar();
    void updateAddressSummary();
    void updatePresetControls();
    void reloadDataFromServer();

    enum class PresetOperations {
        Constant,
        Random,
        Increment,
        Zero
    };

    enum class ValueOperation {
        Set,
        Add,
        Subtract,
        Multiply,
        Divide
    };

    void updateTableWidget();
    QLineEdit* createLineEdit();
    QLineEdit* createNumEdit(int idx);

    template<typename T>
    void applyValue(T value, int index, ValueOperation op)
    {
        switch(op)
        {
        case ValueOperation::Set:       _data[index] = value; break;
        case ValueOperation::Add:       _data[index] += value; break;
        case ValueOperation::Subtract:  _data[index] -= value; break;
        case ValueOperation::Multiply:  _data[index] *= value; break;
        case ValueOperation::Divide:    _data[index] /= value; break;
        }
    }
    void applyToAll(ValueOperation op, double value);

    template<typename T>
    void setupLineEdit(NumericLineEdit* edit, NumericLineEdit::InputMode mode,
                       bool padding = false, T min = std::numeric_limits<T>::lowest() , T max = std::numeric_limits<T>::max())
    {
        edit->setLeadingZeroes(padding);
        edit->setInputMode(mode);

        const QVariant minValue = QVariant::fromValue<T>(min);
        const QVariant maxValue = QVariant::fromValue<T>(max);
        if (minValue.isValid() &&  maxValue.isValid()) {
            edit->setInputRange<T>(min, max);
        }
    }

private:
    Ui::DialogForceMultipleRegisters *ui;
    QVector<quint16> _data;
    ModbusWriteParams& _writeParams;
    QModbusDataUnit::RegisterType _type;
    bool _hexAddress = false;
};

#endif // DIALOGFORCEMULTIPLEREGISTERS_H
