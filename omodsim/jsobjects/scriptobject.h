#ifndef SCRIPTOBJECT_H
#define SCRIPTOBJECT_H

#include <QObject>

///
/// \brief The ScriptObject class
///
class ScriptObject : public QObject
{
    Q_OBJECT
public:
    explicit ScriptObject(QObject *parent = nullptr);

    Q_INVOKABLE void stop();

signals:
    void stopped();
};

#endif // SCRIPTOBJECT_H
