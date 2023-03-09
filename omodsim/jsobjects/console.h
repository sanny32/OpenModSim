#ifndef CONSOLE_H
#define CONSOLE_H

#include <QObject>
#include <QTextDocument>

///
/// \brief The Console class
///
class Console : public QObject
{
    Q_OBJECT
public:
    Console(QTextDocument* doc);

    Q_INVOKABLE void clear();
    Q_INVOKABLE void log(const QString& msg);

private:
    QTextDocument* _document;
};

#endif // CONSOLE_H
