#ifndef SCRIPT_H
#define SCRIPT_H

#include <QObject>

///
/// \brief The Script class
///
class Script : public QObject
{
    Q_OBJECT
public:
    explicit Script(QObject* parent = nullptr);

    Q_INVOKABLE void stop();

signals:
    void stopped();
};

#endif // SCRIPT_H
