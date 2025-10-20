#ifndef DIALOGFORCEMULTIPLEREGISTERS_H
#define DIALOGFORCEMULTIPLEREGISTERS_H

#include <QDialog>
#include <QVariant>
#include <QTableWidgetItem>
#include <QModbusDataUnit>
#include "numericlineedit.h"
#include "modbuswriteparams.h"

namespace Ui {
class DialogForceMultipleRegisters;
}

///
/// \brief The DialogForceMultipleRegisters class
///
class DialogForceMultipleRegisters : public QDialog
{
    Q_OBJECT

public:
    explicit DialogForceMultipleRegisters(ModbusWriteParams& params, QModbusDataUnit::RegisterType type, int length, bool hexAddress, QWidget *parent = nullptr);
    ~DialogForceMultipleRegisters();

    void accept() override;

private slots:
    void on_pushButton0_clicked();
    void on_pushButtonRandom_clicked();
    void on_pushButtonValue_clicked();

private:
    void updateTableWidget();
    QLineEdit* createLineEdit();
    NumericLineEdit* createNumEdit(int idx);

    template<typename T>
    void setupLineEdit(NumericLineEdit* edit, NumericLineEdit::InputMode mode,
                       bool padding = false, T min = std::numeric_limits<T>::lowest() , T max = std::numeric_limits<T>::max())
    {
        edit->setPaddingZeroes(padding);
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
