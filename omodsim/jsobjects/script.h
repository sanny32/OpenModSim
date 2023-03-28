#ifndef SCRIPT_H
#define SCRIPT_H

#include <QObject>
#include <QJSValue>

///
/// \brief The Script class
///
class Script : public QObject
{
    Q_OBJECT
public:
    explicit Script(QObject* parent = nullptr);

    Q_PROPERTY(int runCount READ runCount CONSTANT);
    Q_PROPERTY(int  period READ period CONSTANT)
    Q_INVOKABLE void stop();
    Q_INVOKABLE void onTimeout(int timeout, QJSValue func);

    int runCount() const;
    void setRunCount(int cnt);

    int period() const;
    void setPeriod(int period);

signals:
    void stopped();

private:
    int _runCount = 0;
    int _period = 1000;
};

#endif // SCRIPT_H
