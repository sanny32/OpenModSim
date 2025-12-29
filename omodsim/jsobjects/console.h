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
    explicit console(ConsoleOutput* console);

    Q_INVOKABLE void clear();
    Q_INVOKABLE void log(const QString& msg);
    Q_INVOKABLE void debug(const QString& msg);
    Q_INVOKABLE void warning(const QString& msg);
    Q_INVOKABLE void error(const QString& msg);

private:
    ConsoleOutput* _console;
};

#endif // CONSOLE_H
