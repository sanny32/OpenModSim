#include <QEvent>
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

    setSizeAdjustPolicy(SizeAdjustPolicy::AdjustToContents);
    connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RunModeComboBox::on_currentIndexChanged);
}

///
/// \brief RunModeComboBox::changeEvent
/// \param event
///
void RunModeComboBox::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        for(int i = 0; i < count(); i++)
        {
            switch(itemData(i).value<RunMode>())
            {
                case RunMode::Once:
                    setItemText(i, tr("Once"));
                break;

                case RunMode::Periodically:
                    setItemText(i, tr("Periodically"));
                break;
            }
        }
    }

    QComboBox::changeEvent(event);
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
