#include "scriptobject.h"

///
/// \brief ScriptObject::ScriptObject
/// \param parent
///
ScriptObject::ScriptObject(QObject *parent)
    : QObject{parent}
{

}

///
/// \brief ScriptObject::stop
///
void ScriptObject::stop()
{
    emit stopped();
}
