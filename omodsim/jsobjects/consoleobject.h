#ifndef CONSOLEOBJECT_H
#define CONSOLEOBJECT_H

#include <QObject>
#include <QTextDocument>

///
/// \brief The ConsoleObject class
///
class ConsoleObject : public QObject
{
    Q_OBJECT
public:
    ConsoleObject(QTextDocument* doc);

    Q_INVOKABLE void clear();
    Q_INVOKABLE void log(const QString& msg);

private:
    QTextDocument* _document;
};

#endif // CONSOLEOBJECT_H
