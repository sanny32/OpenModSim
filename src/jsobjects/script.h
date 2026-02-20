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
    explicit Script(int period, QObject* parent = nullptr);

    Q_PROPERTY(int runCount READ runCount CONSTANT);
    Q_PROPERTY(int  period READ period CONSTANT)
    Q_INVOKABLE void stop();
    Q_INVOKABLE void onInit(const QJSValue& func);
    Q_INVOKABLE void setTimeout(const QJSValue& func, int timeout);

    int runCount() const;
    int period() const;

    QJSValue run(QJSEngine& jsEngine, const QString& script);

signals:
    void stopped();

private:
    int _period;
    int _runCount = 0;
};

#endif // SCRIPT_H
