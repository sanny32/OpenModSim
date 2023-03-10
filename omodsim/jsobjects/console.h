#ifndef CONSOLE_H
#define CONSOLE_H

#include <QObject>
#include <QPlainTextEdit>

///
/// \brief The Console class
///
class Console : public QObject
{
    Q_OBJECT
public:
    Console(QPlainTextEdit* edit);

    Q_INVOKABLE void clear();
    Q_INVOKABLE void log(const QString& msg);
    Q_INVOKABLE void error(const QString& msg);

private:
    void addText(const QString& text);
    void setBackgroundColor(const QColor& clr);

private:
    QPlainTextEdit* _edit;
};

#endif // CONSOLE_H
