#ifndef SCRIPTOBJECT_H
#define SCRIPTOBJECT_H

#include <QObject>
#include <QJSValue>

///
/// \brief The ScriptObject class
///
class ScriptObject : public QObject
{
    Q_OBJECT
public:
    explicit ScriptObject(QJSEngine* jsEngine = nullptr);

    Q_INVOKABLE void stop();

signals:
    void stopped();

private:
    QJSEngine* _jsEngine;
};

#endif // SCRIPTOBJECT_H
