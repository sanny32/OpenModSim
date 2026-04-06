#ifndef CONSOLE_H
#define CONSOLE_H

#include <QObject>
#include "consoleoutput.h"

///
/// \brief The console class
///
class console : public QObject
{
    Q_OBJECT
public:
    explicit console(QObject* parent = nullptr);

    Q_INVOKABLE void clear();
    Q_INVOKABLE void log(const QString& msg);
    Q_INVOKABLE void debug(const QString& msg);
    Q_INVOKABLE void warning(const QString& msg);
    Q_INVOKABLE void error(const QString& msg);

signals:
    void messageAdded(const QString& text, ConsoleOutput::MessageType type);
    void clearRequested();
};

#endif // CONSOLE_H

