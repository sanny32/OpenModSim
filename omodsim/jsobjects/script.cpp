#include <QTimer>
#include <QJSEngine>
#include "script.h"

///
/// \brief Script::Script
/// \param period
/// \param parent
///
Script::Script(int period, QObject* parent)
    : QObject(parent)
    ,_period(period)
{
}

///
/// \brief Script::stop
///
void Script::stop()
{
    emit stopped();
}

///
/// \brief Script::run
/// \param jsEngine
/// \param script
/// \return
///
QJSValue Script::run(QJSEngine& jsEngine, const QString& script)
{
    _runCount++;
    return jsEngine.evaluate(script);
}

///
/// \brief Script::onInit
/// \param func
///
void Script::onInit(const QJSValue& func)
{
    if(!func.isCallable())
        return;

    if(_runCount == 1)
        func.call();
}

///
/// \brief Script::setTimeout
/// \param func
/// \param timeout
///
void Script::setTimeout(const QJSValue& func, int timeout)
{
    if(!func.isCallable())
       return;

    QTimer::singleShot(timeout, this, [func]
    {
        func.call();
    });
}

///
/// \brief Script::runCount
/// \return
///
int Script::runCount() const
{
    return _runCount;
}

///
/// \brief Script::period
/// \return
///
int Script::period() const
{
    return _period;
}
