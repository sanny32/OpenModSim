// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file script.cpp
/// \brief Implements the script functionality.
///

#include <QTimer>
#include <QJSValue>
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
        const_cast<QJSValue&>(func).call();
}

///
/// \brief Script::setTimeout
/// \param func
/// \param timeout
/// \return timer id, or 0 if func is not callable
///
int Script::setTimeout(const QJSValue& func, int timeout)
{
    return startTimer(func, timeout, true);
}

///
/// \brief Script::setInterval
/// \param func
/// \param interval
/// \return timer id, or 0 if func is not callable
///
int Script::setInterval(const QJSValue& func, int interval)
{
    return startTimer(func, interval, false);
}

///
/// \brief Script::clearTimeout
/// \param id
///
void Script::clearTimeout(int id)
{
    removeTimer(id);
}

///
/// \brief Script::clearInterval
/// \param id
///
void Script::clearInterval(int id)
{
    removeTimer(id);
}

///
/// \brief Script::startTimer
/// \param func
/// \param interval
/// \param singleShot
/// \return
///
int Script::startTimer(const QJSValue& func, int interval, bool singleShot)
{
    if(!func.isCallable())
        return 0;

    const int id = ++_lastTimerId;

    auto timer = new QTimer(this);
    timer->setSingleShot(singleShot);
    timer->setInterval(qMax(0, interval));

    connect(timer, &QTimer::timeout, this, [this, id, func, singleShot]
    {
        if(singleShot)
        {
            auto expired = _timers.take(id);
            if(expired != nullptr)
                expired->deleteLater();
        }

        const_cast<QJSValue&>(func).call();

        if(singleShot && _timers.isEmpty())
            emit idle();
    });

    _timers.insert(id, timer);
    timer->start();

    return id;
}

///
/// \brief Script::removeTimer
/// \param id
///
void Script::removeTimer(int id)
{
    auto timer = _timers.take(id);
    if(timer == nullptr)
        return;

    timer->stop();
    timer->deleteLater();

    if(_timers.isEmpty())
        emit idle();
}

///
/// \brief Script::hasPendingTimers
/// \return
///
bool Script::hasPendingTimers() const
{
    return !_timers.isEmpty();
}

///
/// \brief Script::stopAllTimers
///
void Script::stopAllTimers()
{
    const auto timers = _timers;
    _timers.clear();

    for(auto timer : timers)
    {
        timer->stop();
        timer->deleteLater();
    }
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

