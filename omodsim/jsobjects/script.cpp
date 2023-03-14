#include "script.h"

///
/// \brief Script::Script
/// \param parent
///
Script::Script(QObject* parent)
    : QObject(parent)
{
}

///
/// \brief Script::stop
///
void Script::stop()
{
    emit stopped();
}
