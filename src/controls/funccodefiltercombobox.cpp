#include <QEvent>
#include <QSignalBlocker>
#include <QModbusPdu>
#include "funccodefiltercombobox.h"

namespace {
constexpr int AllFunctionCode = -1;
}

///
/// \brief FuncCodeFilterComboBox::FuncCodeFilterComboBox
/// \param parent
///
FuncCodeFilterComboBox::FuncCodeFilterComboBox(QWidget* parent)
    : QComboBox(parent)
{
    setSizeAdjustPolicy(QComboBox::AdjustToContents);
    retranslateItems();

    connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FuncCodeFilterComboBox::on_currentIndexChanged);
}

///
/// \brief FuncCodeFilterComboBox::currentFunctionCode
/// \return
///
int FuncCodeFilterComboBox::currentFunctionCode() const
{
    return currentData().toInt();
}

///
/// \brief FuncCodeFilterComboBox::setCurrentFunctionCode
/// \param functionCode
///
void FuncCodeFilterComboBox::setCurrentFunctionCode(int functionCode)
{
    const int idx = findData(functionCode);
    setCurrentIndex(idx < 0 ? 0 : idx);
}

///
/// \brief FuncCodeFilterComboBox::changeEvent
/// \param event
///
void FuncCodeFilterComboBox::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslateItems();

    QComboBox::changeEvent(event);
}

///
/// \brief FuncCodeFilterComboBox::on_currentIndexChanged
/// \param index
///
void FuncCodeFilterComboBox::on_currentIndexChanged(int index)
{
    emit functionCodeChanged(itemData(index).toInt());
}

///
/// \brief FuncCodeFilterComboBox::retranslateItems
///
void FuncCodeFilterComboBox::retranslateItems()
{
    const int currentCode = currentData().toInt();
    const QSignalBlocker blocker(this);

    clear();
    addItem(tr("All"), AllFunctionCode);
    addItem(tr("01 Read Coils"), static_cast<int>(QModbusPdu::ReadCoils));
    addItem(tr("02 Read Discrete Inputs"), static_cast<int>(QModbusPdu::ReadDiscreteInputs));
    addItem(tr("03 Read Holding Registers"), static_cast<int>(QModbusPdu::ReadHoldingRegisters));
    addItem(tr("04 Read Input Registers"), static_cast<int>(QModbusPdu::ReadInputRegisters));
    addItem(tr("05 Write Single Coil"), static_cast<int>(QModbusPdu::WriteSingleCoil));
    addItem(tr("06 Write Single Register"), static_cast<int>(QModbusPdu::WriteSingleRegister));
    addItem(tr("15 Write Multiple Coils"), static_cast<int>(QModbusPdu::WriteMultipleCoils));
    addItem(tr("16 Write Multiple Registers"), static_cast<int>(QModbusPdu::WriteMultipleRegisters));
    addItem(tr("22 Mask Write Register"), static_cast<int>(QModbusPdu::MaskWriteRegister));
    addItem(tr("23 Read/Write Multiple Registers"), static_cast<int>(QModbusPdu::ReadWriteMultipleRegisters));

    setCurrentFunctionCode(currentCode);
}
