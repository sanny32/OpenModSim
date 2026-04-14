#include "paritytypecombobox.h"

///
/// \brief ParityTypeComboBox::ParityTypeComboBox
/// \param parent
///
ParityTypeComboBox::ParityTypeComboBox(QWidget* parent)
    :QComboBox(parent)
{
    retranslateUi();
}

///
/// \brief ParityTypeComboBox::changeEvent
/// \param event
///
void ParityTypeComboBox::changeEvent(QEvent* event)
{
    if(event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QComboBox::changeEvent(event);
}

///
/// \brief ParityTypeComboBox::currentParity
/// \return
///
QSerialPort::Parity ParityTypeComboBox::currentParity() const
{
    return currentData().value<QSerialPort::Parity>();
}

///
/// \brief ParityTypeComboBox::setCurrentParity
/// \param parity
///
void ParityTypeComboBox::setCurrentParity(QSerialPort::Parity parity)
{
    const auto idx = findData(parity);
    setCurrentIndex(idx);
}

///
/// \brief ParityTypeComboBox::retranslateUi
///
void ParityTypeComboBox::retranslateUi()
{
    const auto current = currentParity();
    const QSignalBlocker blocker(this);

    clear();
    addItem(tr("ODD"), QSerialPort::OddParity);
    addItem(tr("EVEN"), QSerialPort::EvenParity);
    addItem(tr("NONE"), QSerialPort::NoParity);

    setCurrentParity(current);
}

