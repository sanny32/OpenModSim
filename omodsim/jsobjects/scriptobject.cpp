#include <QTimer>
#include <QJSEngine>
#include "scriptobject.h"

///
/// \brief ScriptObject::ScriptObject
/// \param jsEngine
///
ScriptObject::ScriptObject(QJSEngine* jsEngine)
    : QObject(jsEngine)
    ,_jsEngine(jsEngine)
{
    Q_ASSERT(jsEngine != nullptr);
}

///
/// \brief ScriptObject::stop
///
void ScriptObject::stop()
{
    emit stopped();
}
