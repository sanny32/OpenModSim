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

///
/// \brief Script::onTimeout
/// \param timeout
/// \param func
///
void Script::onTimeout(int timeout, QJSValue func)
{
   if(!func.isCallable())
       return;

   const auto time = _runCount * _period;
   if(time >= timeout && time <= timeout + _period)
       func.call();
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
/// \brief Script::setRunCount
/// \param cnt
///
void Script::setRunCount(int cnt)
{
    _runCount = qMax(0, cnt);
}

///
/// \brief Script::period
/// \return
///
int Script::period() const
{
    return _period;
}

///
/// \brief Script::setPeriod
/// \param period
///
void Script::setPeriod(int period)
{
    _period = period;
}
