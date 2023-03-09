#include "runmodecombobox.h"

///
/// \brief RunModeComboBox::RunModeComboBox
/// \param parent
///
RunModeComboBox::RunModeComboBox(QWidget* parent)
    : QComboBox(parent)
{
    addItem(tr("Once"), QVariant::fromValue(RunMode::Once));
    addItem(tr("Periodically"), QVariant::fromValue(RunMode::Periodically));

    connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(on_currentIndexChanged(int)));
}

///
/// \brief RunModeComboBox::currentRunMode
/// \return
///
RunMode RunModeComboBox::currentRunMode() const
{
    return currentData().value<RunMode>();
}

///
/// \brief RunModeComboBox::setCurrentRunMode
/// \param mode
///
void RunModeComboBox::setCurrentRunMode(RunMode mode)
{
    const auto idx = findData(QVariant::fromValue(mode));
    setCurrentIndex(idx);
}

///
/// \brief RunModeComboBox::on_currentIndexChanged
/// \param index
///
void RunModeComboBox::on_currentIndexChanged(int index)
{
    emit runModeChanged(itemData(index).value<RunMode>());
}
