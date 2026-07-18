// SPDX-FileCopyrightText: 2026 OpenModSim contributors
// SPDX-License-Identifier: MIT

///
/// \file script.h
/// \brief Declares the script interfaces.
///

#ifndef SCRIPT_H
#define SCRIPT_H

#include <QHash>
#include <QObject>
#include <QJSValue>

class QTimer;

///
/// \brief The Script class
///
class Script : public QObject
{
    Q_OBJECT
public:
    explicit Script(int period, QObject* parent = nullptr);

    Q_PROPERTY(int runCount READ runCount CONSTANT);
    Q_PROPERTY(int  period READ period CONSTANT)
    Q_INVOKABLE void stop();
    Q_INVOKABLE void onInit(const QJSValue& func);
    Q_INVOKABLE int setTimeout(const QJSValue& func, int timeout);
    Q_INVOKABLE int setInterval(const QJSValue& func, int interval);
    Q_INVOKABLE void clearTimeout(int id);
    Q_INVOKABLE void clearInterval(int id);

    int runCount() const;
    int period() const;

    bool hasPendingTimers() const;
    void stopAllTimers();

    QJSValue run(QJSEngine& jsEngine, const QString& script);

signals:
    void stopped();
    void idle();

private:
    int startTimer(const QJSValue& func, int interval, bool singleShot);
    void removeTimer(int id);

private:
    int _period;
    int _runCount = 0;
    int _lastTimerId = 0;
    QHash<int, QTimer*> _timers;
};

#endif // SCRIPT_H

